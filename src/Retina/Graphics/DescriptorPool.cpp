#include <Retina/Graphics/DescriptorPool.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>

#include <volk.h>

namespace Retina::Graphics {
  CDescriptorPool::~CDescriptorPool() noexcept {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_INFO("Descriptor pool ({}) destroyed", GetDebugName());
    vkDestroyDescriptorPool(GetDevice().GetHandle(), _handle, nullptr);
  }

  auto CDescriptorPool::Make(
    const CDevice& device,
    const SDescriptorPoolCreateInfo& createInfo
  ) noexcept -> Core::CArcPtr<CDescriptorPool> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CDescriptorPool());

    auto poolSizes = std::vector<VkDescriptorPoolSize>();
    poolSizes.reserve(createInfo.PoolSizes.size());
    for (const auto& each : createInfo.PoolSizes) {
      auto poolSize = VkDescriptorPoolSize();
      poolSize.type = AsEnumCounterpart(each.Type);
      poolSize.descriptorCount = each.Count;
      poolSizes.push_back(poolSize);
    }

    auto descriptorPoolCreateInfo = VkDescriptorPoolCreateInfo(VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
    descriptorPoolCreateInfo.flags = AsEnumCounterpart(createInfo.Flags);
    descriptorPoolCreateInfo.maxSets = createInfo.MaxSets;
    descriptorPoolCreateInfo.poolSizeCount = poolSizes.size();
    descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();

    auto descriptorPoolHandle = VkDescriptorPool();
    RETINA_GRAPHICS_VULKAN_CHECK(
      vkCreateDescriptorPool(
        device.GetHandle(),
        &descriptorPoolCreateInfo,
        nullptr,
        &descriptorPoolHandle
      )
    );
    RETINA_GRAPHICS_INFO("Descriptor pool ({}) initialized", createInfo.Name);

    self->_handle = descriptorPoolHandle;
    self->_createInfo = createInfo;
    self->_device = device.ToArcPtr();
    self->SetDebugName(createInfo.Name);
  }

  auto CDescriptorPool::Make(
    const CDevice& device,
    uint32 count,
    const SDescriptorPoolCreateInfo& createInfo
  ) noexcept -> std::vector<Core::CArcPtr<CDescriptorPool>> {
    RETINA_PROFILE_SCOPED();
    auto pools = std::vector<Core::CArcPtr<CDescriptorPool>>();
    pools.reserve(count);
    for (auto i = 0_u32; i < count; ++i) {
      pools.emplace_back(Make(device, createInfo));
    }
    return pools;
  }

  auto CDescriptorPool::GetHandle() const noexcept -> VkDescriptorPool {
    RETINA_PROFILE_SCOPED();
    return _handle;
  }

  auto CDescriptorPool::GetCreateInfo() const noexcept -> const SDescriptorPoolCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CDescriptorPool::GetDevice() const noexcept -> const CDevice& {
    RETINA_PROFILE_SCOPED();
    return *_device;
  }

  auto CDescriptorPool::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CDescriptorPool::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_SET_DEBUG_NAME(_device->GetHandle(), _handle, VK_OBJECT_TYPE_DESCRIPTOR_POOL, name);
    _createInfo.Name = name;
  }

  auto CDescriptorPool::Reset() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    vkResetDescriptorPool(GetDevice().GetHandle(), _handle, 0);
  }
}
