#include <Retina/Graphics/BinarySemaphore.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Macros.hpp>
#include <Retina/Graphics/Logger.hpp>

#include <volk.h>

namespace Retina::Graphics {
  CBinarySemaphore::CBinarySemaphore(const CDevice& device) noexcept
    : ISemaphore(device, ESemaphoreKind::E_BINARY)
  {
    RETINA_PROFILE_SCOPED();
  }

  CBinarySemaphore::~CBinarySemaphore() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      RETINA_GRAPHICS_INFO("Binary semaphore ({}) destroyed", GetDebugName());
    }
  }

  auto CBinarySemaphore::Make(
    const CDevice& device,
    const SBinarySemaphoreCreateInfo& createInfo
  ) noexcept -> Core::CArcPtr<CBinarySemaphore> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CBinarySemaphore(device));

    auto semaphoreCreateInfo = VkSemaphoreCreateInfo(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
    auto semaphoreHandle = VkSemaphore();
    RETINA_GRAPHICS_VULKAN_CHECK(vkCreateSemaphore(device.GetHandle(), &semaphoreCreateInfo, nullptr, &semaphoreHandle));
    RETINA_GRAPHICS_INFO("Binary semaphore ({}) initialized", createInfo.Name);

    self->_handle = semaphoreHandle;
    self->_createInfo = createInfo;
    self->SetDebugName(createInfo.Name);
    return self;
  }

  auto CBinarySemaphore::Make(
    const CDevice& device,
    uint32 count,
    const SBinarySemaphoreCreateInfo& createInfo
  ) noexcept -> std::vector<Core::CArcPtr<CBinarySemaphore>> {
    RETINA_PROFILE_SCOPED();
    auto semaphores = std::vector<Core::CArcPtr<CBinarySemaphore>>();
    semaphores.reserve(count);
    for (auto i = 0_u32; i < count; ++i) {
      auto currentCreateInfo = createInfo;
      currentCreateInfo.Name += std::to_string(i);
      semaphores.emplace_back(Make(device, currentCreateInfo));
    }
    return semaphores;
  }

  auto CBinarySemaphore::GetCreateInfo() const noexcept -> const SBinarySemaphoreCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CBinarySemaphore::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CBinarySemaphore::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_SET_DEBUG_NAME(GetDevice().GetHandle(), _handle, VK_OBJECT_TYPE_SEMAPHORE, name);
    _createInfo.Name = name;
  }
}
