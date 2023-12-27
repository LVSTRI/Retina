#include <Retina/Graphics/TimelineSemaphore.hpp>
#include <Retina/Graphics/Device.hpp>

#include <volk.h>

namespace Retina {
    CTimelineSemaphore::CTimelineSemaphore() noexcept : ISemaphore(ESemaphoreKind::E_TIMELINE) {
        RETINA_PROFILE_SCOPED();
    }

    auto CTimelineSemaphore::Make(const CDevice& device, const STimelineSemaphoreCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto semaphore = CArcPtr(new Self());
        RETINA_LOG_INFO(device.GetLogger(), "Creating Timeline Semaphore: \"{}\"", createInfo.Name);
        auto timelineSemaphoreCreateInfo = VkSemaphoreTypeCreateInfo(VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO);
        timelineSemaphoreCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineSemaphoreCreateInfo.initialValue = createInfo.Counter;
        auto semaphoreCreateInfo = VkSemaphoreCreateInfo(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
        semaphoreCreateInfo.pNext = &timelineSemaphoreCreateInfo;

        auto semaphoreHandle = VkSemaphore();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkCreateSemaphore(
                device.GetHandle(),
                AsConstPtr(semaphoreCreateInfo),
                nullptr,
                &semaphoreHandle
            )
        );

        semaphore->_handle = semaphoreHandle;
        semaphore->_device = device.ToArcPtr();
        semaphore->_createInfo = createInfo;
        semaphore->SetDebugName(createInfo.Name);
        return semaphore;
    }

    auto CTimelineSemaphore::Make(
        const CDevice& device,
        uint32 count,
        const STimelineSemaphoreCreateInfo& createInfo
    ) noexcept -> std::vector<CArcPtr<Self>> {
        RETINA_PROFILE_SCOPED();
        auto semaphores = std::vector<CArcPtr<Self>>();
        semaphores.reserve(count);
        for (auto i = 0_u32; i < count; i++) {
            auto newCreateInfo = createInfo;
            newCreateInfo.Name += std::to_string(i);
            semaphores.push_back(Make(device, newCreateInfo));
        }
        return semaphores;
    }

    auto CTimelineSemaphore::GetCreateInfo() const noexcept -> const STimelineSemaphoreCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CTimelineSemaphore::GetCounter() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        auto counter = uint64();
        RETINA_VULKAN_CHECK(
            _device->GetLogger(),
            vkGetSemaphoreCounterValue(
                _device->GetHandle(),
                _handle,
                &counter
            )
        );
        return counter;
    }

    auto CTimelineSemaphore::Wait(uint64 value, uint64 timeout) const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        auto semaphoreWaitInfo = VkSemaphoreWaitInfo(VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO);
        semaphoreWaitInfo.semaphoreCount = 1;
        semaphoreWaitInfo.pSemaphores = &_handle;
        semaphoreWaitInfo.pValues = &value;

        const auto result = vkWaitSemaphores(
            _device->GetHandle(),
            &semaphoreWaitInfo,
            timeout
        );
        if (result == VK_SUCCESS) {
            return true;
        }
        if (result == VK_TIMEOUT) {
            return false;
        }
        RETINA_VULKAN_CHECK(_device->GetLogger(), result);
        std::unreachable();
    }

    auto CTimelineSemaphore::Signal(uint64 value) const noexcept -> void {
        RETINA_PROFILE_SCOPED();
        auto semaphoreSignalInfo = VkSemaphoreSignalInfo(VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO);
        semaphoreSignalInfo.semaphore = _handle;
        semaphoreSignalInfo.value = value;

        RETINA_VULKAN_CHECK(
            _device->GetLogger(),
            vkSignalSemaphore(
                _device->GetHandle(),
                &semaphoreSignalInfo
            )
        );
    }
}
