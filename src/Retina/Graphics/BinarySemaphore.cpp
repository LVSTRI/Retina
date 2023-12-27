#include <Retina/Graphics/BinarySemaphore.hpp>
#include <Retina/Graphics/Device.hpp>

#include <volk.h>

namespace Retina {
    CBinarySemaphore::CBinarySemaphore() noexcept : ISemaphore(ESemaphoreKind::E_BINARY) {
        RETINA_PROFILE_SCOPED();
    }

    auto CBinarySemaphore::Make(const CDevice& device, const SBinarySemaphoreCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto semaphore = CArcPtr(new Self());
        RETINA_LOG_INFO(device.GetLogger(), "Creating Binary Semaphore: \"{}\"", createInfo.Name);
        auto semaphoreHandle = VkSemaphore();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkCreateSemaphore(
                device.GetHandle(),
                AsConstPtr(
                    VkSemaphoreCreateInfo(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO)
                ),
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

    auto CBinarySemaphore::Make(
        const CDevice& device,
        uint32 count,
        const SBinarySemaphoreCreateInfo& createInfo
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

    auto CBinarySemaphore::GetCreateInfo() const noexcept -> const SBinarySemaphoreCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }
}
