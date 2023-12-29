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

    template class CDescriptorTable<EDescriptorType::E_SAMPLED_IMAGE>;
    template class CDescriptorTable<EDescriptorType::E_STORAGE_IMAGE>;
    template class CDescriptorTable<EDescriptorType::E_UNIFORM_BUFFER>;
    template class CDescriptorTable<EDescriptorType::E_STORAGE_BUFFER>;
}
