#include <Retina/Core/External/HashMap.hpp>

#include <Retina/Graphics/RayTracing/TopLevelAccelerationStructure.hpp>
#include <Retina/Graphics/Resources/DescriptorTable.hpp>
#include <Retina/Graphics/Resources/ShaderResourceTable.hpp>
#include <Retina/Graphics/DescriptorLayout.hpp>
#include <Retina/Graphics/DescriptorPool.hpp>
#include <Retina/Graphics/DescriptorSet.hpp>

#include <execution>
#include <algorithm>

namespace Retina {
    template <typename T>
    RETINA_NODISCARD static auto MakeSortedWrites(T&& table) noexcept -> T {
        RETINA_PROFILE_SCOPED();
        if (table.empty()) {
            return {};
        }
        std::sort(std::execution::par, table.begin(), table.end(), [](const auto& x, const auto& y) {
            return x.first < y.first;
        });
        return table;
    }

    template <typename T, typename D = typename T::value_type::second_type>
    RETINA_NODISCARD static auto MergeAdjacentWrites(T&& table) noexcept -> External::FastHashMap<uint64, std::vector<D>> {
        RETINA_PROFILE_SCOPED();
        if (table.empty()) {
            return {};
        }
        auto mergedWrites = External::FastHashMap<uint64, std::vector<D>>();
        {
            const auto& [offset, descriptor] = table[0];
            auto currentOffset = offset;
            mergedWrites[currentOffset].push_back(descriptor);
            for (auto i = 0_u32; i < table.size() - 1; ++i) {
                const auto& [offset, descriptor] = table[i];
                const auto& [nextOffset, nextDescriptor] = table[i + 1];
                mergedWrites[currentOffset].push_back(descriptor);
                if (offset + 1 != nextOffset) {
                    currentOffset = nextOffset;
                }
            }
        }
        return mergedWrites;
    }

    auto CShaderResourceTable::Make(const CDevice& device) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto table = CArcPtr(new Self());
        auto descriptorPoolSizes = std::vector<SDescriptorPoolSize> {
            { Retina::EDescriptorType::E_SAMPLER, 64 },
            { Retina::EDescriptorType::E_SAMPLED_IMAGE, 65536 },
            { Retina::EDescriptorType::E_STORAGE_IMAGE, 65536 },
            { Retina::EDescriptorType::E_UNIFORM_BUFFER, 65536 },
            { Retina::EDescriptorType::E_STORAGE_BUFFER, 65536 },
        };
        if (device.IsExtensionEnabled(&SDeviceExtensionInfo::RayTracing)) {
            descriptorPoolSizes.push_back({ Retina::EDescriptorType::E_ACCELERATION_STRUCTURE_KHR, 1024 });
        }
        auto descriptorPool = Retina::CDescriptorPool::Make(device, {
            .Name = "MainDescriptorPool",
            .Flags = Retina::EDescriptorPoolCreateFlag::E_UPDATE_AFTER_BIND_BIT |
                     Retina::EDescriptorPoolCreateFlag::E_FREE_DESCRIPTOR_SET_BIT,
            .Sizes = std::move(descriptorPoolSizes),
        });
        auto descriptorBindings = std::vector<SDescriptorLayoutBinding> {
            {
                .Count = 64,
                .Stage = Retina::EShaderStage::E_ALL,
                .Type = Retina::EDescriptorType::E_SAMPLER,
                .Flags = Retina::EDescriptorBindingFlag::E_UPDATE_UNUSED_WHILE_PENDING |
                         Retina::EDescriptorBindingFlag::E_PARTIALLY_BOUND,
            },
            {
                .Count = 65536,
                .Stage = Retina::EShaderStage::E_ALL,
                .Type = Retina::EDescriptorType::E_SAMPLED_IMAGE,
                .Flags = Retina::EDescriptorBindingFlag::E_UPDATE_UNUSED_WHILE_PENDING |
                         Retina::EDescriptorBindingFlag::E_PARTIALLY_BOUND,
            },
            {
                .Count = 65536,
                .Stage = Retina::EShaderStage::E_ALL,
                .Type = Retina::EDescriptorType::E_STORAGE_IMAGE,
                .Flags = Retina::EDescriptorBindingFlag::E_UPDATE_UNUSED_WHILE_PENDING |
                         Retina::EDescriptorBindingFlag::E_PARTIALLY_BOUND,
            },
            {
                .Count = 65536,
                .Stage = Retina::EShaderStage::E_ALL,
                .Type = Retina::EDescriptorType::E_UNIFORM_BUFFER,
                .Flags = Retina::EDescriptorBindingFlag::E_UPDATE_UNUSED_WHILE_PENDING |
                         Retina::EDescriptorBindingFlag::E_PARTIALLY_BOUND,
            },
            {
                .Count = 65536,
                .Stage = Retina::EShaderStage::E_ALL,
                .Type = Retina::EDescriptorType::E_STORAGE_BUFFER,
                .Flags = Retina::EDescriptorBindingFlag::E_UPDATE_UNUSED_WHILE_PENDING |
                         Retina::EDescriptorBindingFlag::E_PARTIALLY_BOUND,
            }
        };
        if (device.IsExtensionEnabled(&SDeviceExtensionInfo::RayTracing)) {
            descriptorBindings.push_back({
                .Count = 1024,
                .Stage = Retina::EShaderStage::E_ALL,
                .Type = Retina::EDescriptorType::E_ACCELERATION_STRUCTURE_KHR,
                .Flags = Retina::EDescriptorBindingFlag::E_UPDATE_UNUSED_WHILE_PENDING |
                         Retina::EDescriptorBindingFlag::E_PARTIALLY_BOUND,
            });
        }
        auto descriptorLayout = Retina::CDescriptorLayout::Make(*descriptorPool, {
            .Name = "MainDescriptorLayout",
            .Bindings = std::move(descriptorBindings),
        });
        auto descriptorSet = Retina::CDescriptorSet::Make(device, {
            .Name = "MainDescriptorSet",
            .Layout = {
                std::move(descriptorLayout),
            }
        });
        table->_device = device.ToArcPtr();
        table->_descriptorSet = std::move(descriptorSet);
        return table;
    }

    auto CShaderResourceTable::GetSamplerTable() noexcept -> CDescriptorTable<EDescriptorType::E_SAMPLER>& {
        RETINA_PROFILE_SCOPED();
        return _samplerTable;
    }

    auto CShaderResourceTable::GetSampledImageTable() noexcept -> CDescriptorTable<EDescriptorType::E_SAMPLED_IMAGE>& {
        RETINA_PROFILE_SCOPED();
        return _sampledImageTable;
    }

    auto CShaderResourceTable::GetStorageImageTable() noexcept -> CDescriptorTable<EDescriptorType::E_STORAGE_IMAGE>& {
        RETINA_PROFILE_SCOPED();
        return _storageImageTable;
    }

    auto CShaderResourceTable::GetUniformBufferTable() noexcept -> CDescriptorTable<EDescriptorType::E_UNIFORM_BUFFER>& {
        RETINA_PROFILE_SCOPED();
        return _uniformBufferTable;
    }

    auto CShaderResourceTable::GetStorageBufferTable() noexcept -> CDescriptorTable<EDescriptorType::E_STORAGE_BUFFER>& {
        RETINA_PROFILE_SCOPED();
        return _storageBufferTable;
    }

    auto CShaderResourceTable::GetAccelerationStructureTable() noexcept -> CDescriptorTable<EDescriptorType::E_ACCELERATION_STRUCTURE_KHR>& {
        RETINA_PROFILE_SCOPED();
        return _accelerationStructureTable;
    }

    auto CShaderResourceTable::GetDevice() const noexcept -> const CDevice& {
        RETINA_PROFILE_SCOPED();
        return *_device;
    }

    auto CShaderResourceTable::GetDescriptorLayout() const noexcept -> const CDescriptorLayout& {
        RETINA_PROFILE_SCOPED();
        return GetDescriptorSet().GetLayout();
    }

    auto CShaderResourceTable::GetDescriptorPool() const noexcept -> const CDescriptorPool& {
        RETINA_PROFILE_SCOPED();
        return GetDescriptorSet().GetDescriptorPool();
    }

    auto CShaderResourceTable::GetDescriptorSet() const noexcept -> const CDescriptorSet& {
        RETINA_PROFILE_SCOPED();
        return *_descriptorSet;
    }

    auto CShaderResourceTable::MakeSampler(const SSamplerCreateInfo& info) noexcept -> SamplerResource {
        RETINA_PROFILE_SCOPED();
        auto sampler = CSampler::Make(*_device, info);
        const auto index = _samplerTable.AllocateResource(sampler);
        return SamplerResource::Make(*this, std::move(sampler), index);
    }

    auto CShaderResourceTable::MakeSampledImage(const SImageCreateInfo& info) noexcept -> SampledImageResource {
        RETINA_PROFILE_SCOPED();
        auto image = CImage::Make(*_device, info);
        const auto index = _sampledImageTable.AllocateResource(image);
        return SampledImageResource::Make(*this, std::move(image), index);
    }

    auto CShaderResourceTable::MakeStorageImage(const SImageCreateInfo& info) noexcept -> StorageImageResource {
        RETINA_PROFILE_SCOPED();
        auto image = CImage::Make(*_device, info);
        const auto index = _storageImageTable.AllocateResource(image);
        return StorageImageResource::Make(*this, std::move(image), index);
    }

    auto CShaderResourceTable::MakeAccelerationStructure(const SAccelerationStructureCreateInfo& info) noexcept -> AccelerationStructureResource {
        RETINA_PROFILE_SCOPED();
        auto accelerationStructure = CTopLevelAccelerationStructure::Make(*_device, info);
        const auto index = _accelerationStructureTable.AllocateResource(accelerationStructure);
        return AccelerationStructureResource::Make(*this, std::move(accelerationStructure), index);
    }

    auto CShaderResourceTable::Update() noexcept -> void {
        RETINA_PROFILE_SCOPED();
        auto&& samplerWrites = GetSamplerTable().GetWrites();
        auto&& sampledImageWrites = GetSampledImageTable().GetWrites();
        auto&& storageImageWrites = GetStorageImageTable().GetWrites();
        auto&& uniformBufferWrites = GetUniformBufferTable().GetWrites();
        auto&& storageBufferWrites = GetStorageBufferTable().GetWrites();
        auto&& accelerationStructureWrites = GetAccelerationStructureTable().GetWrites();

        auto descriptorWrites = std::vector<SDescriptorWriteInfo>();
        if (!samplerWrites.empty()) {
            for (auto&& [index, descriptors] : MergeAdjacentWrites(MakeSortedWrites(std::move(samplerWrites)))) {
                descriptorWrites.push_back({
                    .Slot = static_cast<uint32>(index),
                    .Type = EDescriptorType::E_SAMPLER,
                    .Descriptors = std::move(descriptors),
                });
            }
        }
        if (!sampledImageWrites.empty()) {
            for (auto&& [index, descriptors] : MergeAdjacentWrites(MakeSortedWrites(std::move(sampledImageWrites)))) {
                descriptorWrites.push_back({
                    .Slot = static_cast<uint32>(index),
                    .Type = EDescriptorType::E_SAMPLED_IMAGE,
                    .Descriptors = std::move(descriptors),
                });
            }
        }
        if (!storageImageWrites.empty()) {
            for (auto&& [index, descriptors] : MergeAdjacentWrites(MakeSortedWrites(std::move(storageImageWrites)))) {
                descriptorWrites.push_back({
                    .Slot = static_cast<uint32>(index),
                    .Type = EDescriptorType::E_STORAGE_IMAGE,
                    .Descriptors = std::move(descriptors),
                });
            }
        }
        if (!uniformBufferWrites.empty()) {
            for (auto&& [index, descriptors] : MergeAdjacentWrites(MakeSortedWrites(std::move(uniformBufferWrites)))) {
                descriptorWrites.push_back({
                    .Slot = static_cast<uint32>(index),
                    .Type = EDescriptorType::E_UNIFORM_BUFFER,
                    .Descriptors = std::move(descriptors),
                });
            }
        }
        if (!storageBufferWrites.empty()) {
            for (auto&& [index, descriptors] : MergeAdjacentWrites(MakeSortedWrites(std::move(storageBufferWrites)))) {
                descriptorWrites.push_back({
                    .Slot = static_cast<uint32>(index),
                    .Type = EDescriptorType::E_STORAGE_BUFFER,
                    .Descriptors = std::move(descriptors),
                });
            }
        }
        if (!accelerationStructureWrites.empty()) {
            for (auto&& [index, descriptors] : MergeAdjacentWrites(MakeSortedWrites(std::move(accelerationStructureWrites)))) {
                descriptorWrites.push_back({
                    .Slot = static_cast<uint32>(index),
                    .Type = EDescriptorType::E_ACCELERATION_STRUCTURE_KHR,
                    .Descriptors = std::move(descriptors),
                });
            }
        }

        if (!descriptorWrites.empty()) {
            GetDescriptorSet().Write(descriptorWrites);
        }
    }
}
