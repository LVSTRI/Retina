#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Fence.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>

#include <volk.h>

namespace Retina::Graphics {
  CFence::~CFence() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      vkDestroyFence(_device->GetHandle(), _handle, nullptr);
      RETINA_GRAPHICS_INFO("Fence ({}) destroyed", GetDebugName());
    }
  }

  auto CFence::Make(const CDevice& device, const SFenceCreateInfo& createInfo) noexcept -> Core::CArcPtr<CFence> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CFence());
    auto fenceCreateInfo = VkFenceCreateInfo(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
    fenceCreateInfo.flags = createInfo.IsSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    auto fenceHandle = VkFence();
    RETINA_GRAPHICS_VULKAN_CHECK(vkCreateFence(device.GetHandle(), &fenceCreateInfo, nullptr, &fenceHandle));
    RETINA_GRAPHICS_INFO("Fence ({}) initialized", createInfo.Name);

    self->_handle = fenceHandle;
    self->_createInfo = createInfo;
    self->_device = device.ToArcPtr();
    self->SetDebugName(createInfo.Name);

    return self;
  }

  auto CFence::Make(
    const CDevice& device,
    uint32 count,
    const SFenceCreateInfo& createInfo
  ) noexcept -> std::vector<Core::CArcPtr<CFence>> {
    RETINA_PROFILE_SCOPED();
    auto fences = std::vector<Core::CArcPtr<CFence>>();
    fences.reserve(count);
    for (auto i = 0_u32; i < count; ++i) {
      fences.emplace_back(Make(device, createInfo));
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

  auto CFence::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CFence::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_DEBUG_NAME(_device->GetHandle(), _handle, VK_OBJECT_TYPE_FENCE, name);
    _createInfo.Name = name;
  }

  auto CFence::IsReady() const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    const auto result = vkGetFenceStatus(_device->GetHandle(), _handle);
    switch (result) {
      case VK_SUCCESS:
        return true;
      case VK_NOT_READY:
        return false;
      default:
        RETINA_GRAPHICS_VULKAN_CHECK(result);
        std::unreachable();
    }
  }

  auto CFence::Reset() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_VULKAN_CHECK(vkResetFences(_device->GetHandle(), 1, &_handle));
  }

  auto CFence::Wait(uint64 timeout) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_VULKAN_CHECK(vkWaitForFences(_device->GetHandle(), 1, &_handle, true, timeout));
  }
}
