#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/DescriptorLayoutInfo.hpp>

#include <vulkan/vulkan.h>

namespace Retina {
    class CDescriptorLayout : public INativeDebugName, public IEnableIntrusiveReferenceCount<CDescriptorLayout> {
    public:
        using Self = CDescriptorLayout;

        CDescriptorLayout() noexcept = default;
        ~CDescriptorLayout() noexcept;

        RETINA_NODISCARD static auto Make(const CDescriptorPool& descriptorPool, const SDescriptorLayoutCreateInfo& createInfo) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkDescriptorSetLayout;
        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SDescriptorLayoutCreateInfo&;
        RETINA_NODISCARD auto GetDescriptorPool() const noexcept -> const CDescriptorPool&;
        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

        auto SetDebugName(std::string_view name) noexcept -> void;

        RETINA_NODISCARD auto GetBinding(uint32 binding) const noexcept -> const SDescriptorLayoutBinding&;

    private:
        VkDescriptorSetLayout _handle = {};

        SDescriptorLayoutCreateInfo _createInfo = {};
        CArcPtr<const CDescriptorPool> _descriptorPool;
    };
}
