#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Fence.hpp>

#include <volk.h>

namespace Retina {
    CFence::~CFence() noexcept {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(_device->GetLogger(), "Destroying Fence: \"{}\"", GetDebugName());
        vkDestroyFence(_device->GetHandle(), _handle, nullptr);
    }

    auto CFence::Make(const CDevice& device, const SFenceCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto fence = CArcPtr(new Self());
        RETINA_LOG_INFO(device.GetLogger(), "Creating Fence: \"{}\"", createInfo.Name);
        auto fenceCreateInfo = VkFenceCreateInfo(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
        fenceCreateInfo.flags = createInfo.IsSignaled
            ? VK_FENCE_CREATE_SIGNALED_BIT
            : 0;
        auto fenceHandle = VkFence();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkCreateFence(
                device.GetHandle(),
                &fenceCreateInfo,
                nullptr,
                &fenceHandle
            )
        );

        fence->_handle = fenceHandle;
        fence->_createInfo = createInfo;
        fence->_device = device.ToArcPtr();
        fence->SetDebugName(createInfo.Name);
        return fence;
    }

    auto CFence::Make(
        const CDevice& device,
        uint32 count,
        const SFenceCreateInfo& createInfo
    ) noexcept -> std::vector<CArcPtr<Self>> {
        RETINA_PROFILE_SCOPED();
        auto fences = std::vector<CArcPtr<Self>>();
        fences.reserve(count);
        for (auto i = 0_u32; i < count; i++) {
            auto newCreateInfo = createInfo;
            newCreateInfo.Name += std::to_string(i);
            fences.push_back(Make(device, newCreateInfo));
        }
        return fences;
    }

    auto CFence::GetHandle() const noexcept -> VkFence {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto CFence::GetCreateInfo() const noexcept -> const SFenceCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CFence::GetDevice() const noexcept -> const CDevice& {
        RETINA_PROFILE_SCOPED();
        return *_device;
    }

    auto CFence::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_FENCE;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(_device->GetLogger(), vkSetDebugUtilsObjectNameEXT(_device->GetHandle(), &info));
    }

    auto CFence::IsReady() const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        const auto result = vkGetFenceStatus(_device->GetHandle(), _handle);
        switch (result) {
            case VK_SUCCESS: return true;
            case VK_NOT_READY: return false;
            default: break;
        }
        RETINA_VULKAN_CHECK(_device->GetLogger(), result);
        std::unreachable();
    }

    auto CFence::Reset() const noexcept -> void {
        RETINA_PROFILE_SCOPED();
        RETINA_VULKAN_CHECK(
            _device->GetLogger(),
            vkResetFences(
                _device->GetHandle(),
                1,
                &_handle
            )
        );
    }

    auto CFence::Wait(uint64 timeout) const noexcept -> void {
        RETINA_PROFILE_SCOPED();
        RETINA_VULKAN_CHECK(
            _device->GetLogger(),
            vkWaitForFences(
                _device->GetHandle(),
                1,
                &_handle,
                true,
                timeout
            )
        );
    }
}
