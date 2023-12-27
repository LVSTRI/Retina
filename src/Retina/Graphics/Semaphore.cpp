#include <Retina/Graphics/Semaphore.hpp>
#include <Retina/Graphics/Device.hpp>

#include <volk.h>

namespace Retina {
    ISemaphore::ISemaphore(ESemaphoreKind kind) noexcept : _kind(kind) {
        RETINA_PROFILE_SCOPED();
    }

    ISemaphore::~ISemaphore() noexcept {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(_device->GetLogger(), "Destroying Semaphore: \"{}\"", GetDebugName());
        vkDestroySemaphore(_device->GetHandle(), _handle, nullptr);
    }

    auto ISemaphore::GetHandle() const noexcept -> VkSemaphore {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto ISemaphore::GetKind() const noexcept -> ESemaphoreKind {
        RETINA_PROFILE_SCOPED();
        return _kind;
    }

    auto ISemaphore::GetDevice() const noexcept -> const CDevice& {
        RETINA_PROFILE_SCOPED();
        return *_device;
    }

    auto ISemaphore::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(_device->GetLogger(), vkSetDebugUtilsObjectNameEXT(_device->GetHandle(), &info));
    }
}
