#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>
#include <Retina/Graphics/TimelineSemaphore.hpp>

#include <volk.h>

namespace Retina::Graphics {
  CTimelineSemaphore::CTimelineSemaphore(const CDevice& device) noexcept
    : ISemaphore(device, ESemaphoreKind::E_TIMELINE)
  {
    RETINA_PROFILE_SCOPED();
  }

  CTimelineSemaphore::~CTimelineSemaphore() noexcept {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_INFO("Timeline semaphore ({}) destroyed", GetDebugName());
  }

  auto CTimelineSemaphore::Make(
    const CDevice& device,
    const STimelineSemaphoreCreateInfo& createInfo
  ) noexcept -> Core::CArcPtr<CTimelineSemaphore> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CTimelineSemaphore(device));
    auto timelineSemaphoreCreateInfo = VkSemaphoreTypeCreateInfo(VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO);
    timelineSemaphoreCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineSemaphoreCreateInfo.initialValue = createInfo.Value;
    auto semaphoreCreateInfo = VkSemaphoreCreateInfo(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
    semaphoreCreateInfo.pNext = &timelineSemaphoreCreateInfo;

    auto semaphoreHandle = VkSemaphore();
    RETINA_GRAPHICS_VULKAN_CHECK(vkCreateSemaphore(device.GetHandle(), &semaphoreCreateInfo, nullptr, &semaphoreHandle));
    RETINA_GRAPHICS_INFO("Timeline semaphore ({}) initialized", createInfo.Name);

    self->_handle = semaphoreHandle;
    self->_createInfo = createInfo;
    self->SetDebugName(createInfo.Name);

    return self;
  }

  auto CTimelineSemaphore::Make(
    const CDevice& device,
    uint32 count,
    const STimelineSemaphoreCreateInfo& createInfo
  ) noexcept -> std::vector<Core::CArcPtr<CTimelineSemaphore>> {
    RETINA_PROFILE_SCOPED();
    auto semaphores = std::vector<Core::CArcPtr<CTimelineSemaphore>>();
    semaphores.reserve(count);
    for (auto i = 0_u32; i < count; ++i) {
      auto currentCreateInfo = createInfo;
      currentCreateInfo.Name = std::format("{}{}", createInfo.Name, i);
      semaphores.emplace_back(Make(device, currentCreateInfo));
    }
    return semaphores;
  }

  auto CTimelineSemaphore::GetCreateInfo() const noexcept -> const STimelineSemaphoreCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CTimelineSemaphore::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CTimelineSemaphore::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_SET_DEBUG_NAME(GetDevice().GetHandle(), _handle, VK_OBJECT_TYPE_SEMAPHORE, name);
    _createInfo.Name = name;
  }

  auto CTimelineSemaphore::GetCounter() const noexcept -> uint64 {
    RETINA_PROFILE_SCOPED();
    auto value = 0_u64;
    RETINA_GRAPHICS_VULKAN_CHECK(vkGetSemaphoreCounterValue(GetDevice().GetHandle(), _handle, &value));
    return value;
  }

  auto CTimelineSemaphore::Wait(uint64 value, uint64 timeout) const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    auto semaphoreWaitInfo = VkSemaphoreWaitInfo(VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO);
    semaphoreWaitInfo.semaphoreCount = 1;
    semaphoreWaitInfo.pSemaphores = &_handle;
    semaphoreWaitInfo.pValues = &value;

    const auto result = vkWaitSemaphores(GetDevice().GetHandle(), &semaphoreWaitInfo, timeout);
    switch (result) {
      case VK_SUCCESS:
        return true;
      case VK_TIMEOUT:
        return false;
      default:
        RETINA_GRAPHICS_VULKAN_CHECK(result);
        std::unreachable();
    }
  }

  auto CTimelineSemaphore::Signal(uint64 value) const noexcept -> void {
    RETINA_PROFILE_SCOPED();
    auto semaphoreSignalInfo = VkSemaphoreSignalInfo(VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO);
    semaphoreSignalInfo.semaphore = _handle;
    semaphoreSignalInfo.value = value;

    RETINA_GRAPHICS_VULKAN_CHECK(vkSignalSemaphore(GetDevice().GetHandle(), &semaphoreSignalInfo));
  }
}
