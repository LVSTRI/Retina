#include <Retina/Graphics/Resources/DescriptorTable.hpp>

namespace Retina {
    template <EDescriptorType D>
    CDescriptorTable<D>::CDescriptorTable() noexcept {
        RETINA_PROFILE_SCOPED();
        _resources.reserve(65536);
        _free.reserve(65536);
        _writes.reserve(65536);
    }

    template <EDescriptorType D>
    auto CDescriptorTable<D>::AllocateResource(
        CArcPtr<CSampler> sampler
    ) noexcept -> uint32
        requires (std::same_as<Resource, CSampler>)
    {
        RETINA_PROFILE_SCOPED();
        const auto index = GetFreeIndex();
        _writes.emplace_back(index, sampler->GetDescriptor());
        _resources[index] = std::move(sampler);
        return index;
    }

    template <EDescriptorType D>
    auto CDescriptorTable<D>::AllocateResource(
        CArcPtr<CBuffer> buffer
    ) noexcept -> uint32
        requires (std::same_as<Resource, CBuffer>)
    {
        RETINA_PROFILE_SCOPED();
        const auto index = GetFreeIndex();
        _writes.emplace_back(index, buffer->GetDescriptor());
        _resources[index] = std::move(buffer);
        return index;
    }

    template <EDescriptorType D>
    auto CDescriptorTable<D>::AllocateResource(
        std::span<CArcPtr<CBuffer>> buffers
    ) noexcept -> std::vector<uint32>
        requires (std::same_as<Resource, CBuffer>)
    {
        RETINA_PROFILE_SCOPED();
        auto indices = std::vector<uint32>();
        indices.reserve(buffers.size());
        for (auto&& buffer : buffers) {
            const auto index = GetFreeIndex();
            _writes.emplace_back(index, buffer->GetDescriptor());
            _resources[index] = std::move(buffer);
            indices.emplace_back(index);
        }
        return indices;
    }

    template <EDescriptorType D>
    auto CDescriptorTable<D>::AllocateResource(
        CArcPtr<CImage> image,
        EImageLayout layout
    ) noexcept -> uint32
        requires (std::same_as<Resource, CImage>)
    {
        RETINA_PROFILE_SCOPED();
        const auto index = GetFreeIndex();
        _writes.emplace_back(index, image->GetDescriptor(layout));
        _resources[index] = std::move(image);
        return index;
    }

    template <EDescriptorType D>
    auto CDescriptorTable<D>::AllocateResource(
        CArcPtr<CTopLevelAccelerationStructure> accelerationStructure
    ) noexcept -> uint32
        requires (std::same_as<Resource, CTopLevelAccelerationStructure>)
    {
        RETINA_PROFILE_SCOPED();
        const auto index = GetFreeIndex();
        _writes.emplace_back(index, accelerationStructure->GetDescriptor());
        _resources[index] = std::move(accelerationStructure);
        return index;
    }

    template <EDescriptorType D>
    auto CDescriptorTable<D>::FreeResource(uint32 index) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        _resources[index] = {};
        _free.emplace_back(index);
    }

    template <EDescriptorType D>
    auto CDescriptorTable<D>::GetWrites() -> std::vector<std::pair<uint32, Descriptor>>&& {
        RETINA_PROFILE_SCOPED();
        return std::move(_writes);
    }

    template <EDescriptorType D>
    auto CDescriptorTable<D>::GetFreeIndex() noexcept -> uint32 {
        RETINA_PROFILE_SCOPED();
        if (_free.empty()) {
            const auto index = _resources.size();
            _resources.emplace_back();
            return index;
        }
        const auto index = _free.back();
        _free.pop_back();
        return index;
    }

    template class CDescriptorTable<EDescriptorType::E_SAMPLER>;
    template class CDescriptorTable<EDescriptorType::E_SAMPLED_IMAGE>;
    template class CDescriptorTable<EDescriptorType::E_STORAGE_IMAGE>;
    template class CDescriptorTable<EDescriptorType::E_STORAGE_BUFFER>;
    template class CDescriptorTable<EDescriptorType::E_ACCELERATION_STRUCTURE_KHR>;
}
