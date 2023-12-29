#include <Retina/Core/External/HashMap.hpp>

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
        for (const auto& [index, descriptor] : table) {
            if (mergedWrites.contains(index)) {
                mergedWrites[index].emplace_back(descriptor);
            } else {
                mergedWrites[index] = { descriptor };
            }
        }
        return mergedWrites;
    }

    auto CShaderResourceTable::Make(const CDevice& device) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto table = CArcPtr(new Self());
        auto descriptorPool = Retina::CDescriptorPool::Make(device, {
            .Name = "MainDescriptorPool",
            .Flags = Retina::EDescriptorPoolCreateFlag::E_UPDATE_AFTER_BIND_BIT |
                     Retina::EDescriptorPoolCreateFlag::E_FREE_DESCRIPTOR_SET_BIT,
            .Sizes = {
                { Retina::EDescriptorType::E_SAMPLER, 64 },
                { Retina::EDescriptorType::E_SAMPLED_IMAGE, 65536 },
                { Retina::EDescriptorType::E_STORAGE_IMAGE, 65536 },
                { Retina::EDescriptorType::E_UNIFORM_BUFFER, 65536 },
                { Retina::EDescriptorType::E_STORAGE_BUFFER, 65536 },
            },
        });
        auto descriptorLayout = Retina::CDescriptorLayout::Make(*descriptorPool, {
            .Name = "MainDescriptorLayout",
            .Bindings = {
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
                },
            },
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

    auto CShaderResourceTable::Update() noexcept -> void {
        RETINA_PROFILE_SCOPED();
        auto sampledImageWrites = MergeAdjacentWrites(MakeSortedWrites(GetSampledImageTable().GetWrites()));
        auto storageImageWrites = MergeAdjacentWrites(MakeSortedWrites(GetStorageImageTable().GetWrites()));
        auto uniformBufferWrites = MergeAdjacentWrites(MakeSortedWrites(GetUniformBufferTable().GetWrites()));
        auto storageBufferWrites = MergeAdjacentWrites(MakeSortedWrites(GetStorageBufferTable().GetWrites()));

        auto descriptorWrites = std::vector<SDescriptorWriteInfo>();
        if (!sampledImageWrites.empty()) {
            for (auto&& [index, descriptors] : sampledImageWrites) {
                descriptorWrites.push_back({
                    .Slot = static_cast<uint32>(index),
                    .Type = EDescriptorType::E_SAMPLED_IMAGE,
                    .Descriptors = std::move(descriptors),
                });
            }
        }
        if (!storageImageWrites.empty()) {
            for (auto&& [index, descriptors] : storageImageWrites) {
                descriptorWrites.push_back({
                    .Slot = static_cast<uint32>(index),
                    .Type = EDescriptorType::E_STORAGE_IMAGE,
                    .Descriptors = std::move(descriptors),
                });
            }
        }
        if (!uniformBufferWrites.empty()) {
            for (auto&& [index, descriptors] : uniformBufferWrites) {
                descriptorWrites.push_back({
                    .Slot = static_cast<uint32>(index),
                    .Type = EDescriptorType::E_UNIFORM_BUFFER,
                    .Descriptors = std::move(descriptors),
                });
            }
        }
        if (!storageBufferWrites.empty()) {
            for (auto&& [index, descriptors] : storageBufferWrites) {
                descriptorWrites.push_back({
                    .Slot = static_cast<uint32>(index),
                    .Type = EDescriptorType::E_STORAGE_BUFFER,
                    .Descriptors = std::move(descriptors),
                });
            }
        }
        if (!descriptorWrites.empty()) {
            GetDescriptorSet().Write(descriptorWrites);
        }
    }
}
