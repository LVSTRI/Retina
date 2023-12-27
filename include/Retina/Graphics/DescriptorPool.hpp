#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/DescriptorPoolInfo.hpp>

#include <vulkan/vulkan.h>

namespace Retina {
    class CDescriptorPool : public INativeDebugName, public IEnableIntrusiveReferenceCount<CDescriptorPool> {
    public:
        using Self = CDescriptorPool;

        CDescriptorPool() noexcept = default;
        ~CDescriptorPool() noexcept;

        RETINA_NODISCARD static auto Make(const CDevice& device, const SDescriptorPoolCreateInfo& createInfo) noexcept -> CArcPtr<Self>;
        RETINA_NODISCARD static auto Make(
            const CDevice& device,
            uint32 count,
            const SDescriptorPoolCreateInfo& createInfo
        ) noexcept -> std::vector<CArcPtr<Self>>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkDescriptorPool;
        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SDescriptorPoolCreateInfo&;
        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

        auto SetDebugName(std::string_view name) noexcept -> void;

        auto Reset() const noexcept -> void;

    private:
        VkDescriptorPool _handle = {};

        SDescriptorPoolCreateInfo _createInfo = {};
        CArcPtr<const CDevice> _device;
    };
}