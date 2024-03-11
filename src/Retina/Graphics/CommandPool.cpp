#include <Retina/Graphics/CommandPool.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>
#include <Retina/Graphics/Queue.hpp>

#include <volk.h>

namespace Retina::Graphics {
  CCommandPool::~CCommandPool() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      vkDestroyCommandPool(GetQueue().GetDevice().GetHandle(), _handle, nullptr);
      RETINA_GRAPHICS_INFO("Command pool ({}) destroyed", GetDebugName());
    }
  }

  auto CCommandPool::Make(
    const CQueue& queue,
    const SCommandPoolCreateInfo& createInfo
  ) noexcept -> Core::CArcPtr<CCommandPool> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CCommandPool());
    auto commandPoolCreateInfo = VkCommandPoolCreateInfo(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
    commandPoolCreateInfo.flags = AsEnumCounterpart(createInfo.Flags);
    commandPoolCreateInfo.queueFamilyIndex = queue.GetFamilyIndex();

    auto commandPoolHandle = VkCommandPool();
    RETINA_GRAPHICS_VULKAN_CHECK(
      vkCreateCommandPool(
        queue.GetDevice().GetHandle(),
        &commandPoolCreateInfo,
        nullptr,
        &commandPoolHandle
      )
    );
    RETINA_GRAPHICS_INFO("Command pool ({}) initialized", createInfo.Name);

    self->_handle = commandPoolHandle;
    self->_createInfo = createInfo;
    self->_queue = queue.ToArcPtr();
    self->SetDebugName(createInfo.Name);

    return self;
  }

  auto CCommandPool::Make(
    const CQueue& queue,
    uint32 count,
    const SCommandPoolCreateInfo& createInfo
  ) noexcept -> std::vector<Core::CArcPtr<CCommandPool>> {
    RETINA_PROFILE_SCOPED();
    auto commandPools = std::vector<Core::CArcPtr<CCommandPool>>();
    commandPools.reserve(count);
    for (auto i = 0_u32; i < count; ++i) {
      commandPools.emplace_back(Make(queue, createInfo));
    }
    return commandPools;
  }

  auto CCommandPool::GetHandle() const noexcept -> VkCommandPool {
    RETINA_PROFILE_SCOPED();
    return _handle;
  }

  auto CCommandPool::GetCreateInfo() const noexcept -> const SCommandPoolCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CCommandPool::GetQueue() const noexcept -> const CQueue& {
    RETINA_PROFILE_SCOPED();
    return *_queue;
  }

  auto CCommandPool::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CCommandPool::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_DEBUG_NAME(
      GetQueue().GetDevice().GetHandle(),
      _handle,
      VK_OBJECT_TYPE_COMMAND_POOL,
      name
    );
    _createInfo.Name = name;
  }

  auto CCommandPool::Reset() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_VULKAN_CHECK(
      vkResetCommandPool(
        GetQueue().GetDevice().GetHandle(),
        _handle,
        0
      )
    );
  }
}
