#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/FenceInfo.hpp>

#include <vulkan/vulkan.h>

namespace Retina {
    class CFence : public INativeDebugName, public IEnableIntrusiveReferenceCount<CFence> {
    public:
        using Self = CFence;

        CFence() noexcept = default;
        ~CFence() noexcept;

        RETINA_NODISCARD static auto Make(const CDevice& device, const SFenceCreateInfo& createInfo) noexcept -> CArcPtr<Self>;
        RETINA_NODISCARD static auto Make(
            const CDevice& device,
            uint32 count,
            const SFenceCreateInfo& createInfo
        ) noexcept -> std::vector<CArcPtr<Self>>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkFence;
        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SFenceCreateInfo&;
        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

        auto SetDebugName(std::string_view name) noexcept -> void;

        RETINA_NODISCARD auto IsReady() const noexcept -> bool;

        auto Reset() const noexcept -> void;
        auto Wait(uint64 timeout = -1_u64) const noexcept -> void;

    private:
        VkFence _handle = {};

        SFenceCreateInfo _createInfo = {};
        CArcPtr<const CDevice> _device;
    };
}
