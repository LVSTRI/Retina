#include <Retina/Graphics/DescriptorLayout.hpp>
#include <Retina/Graphics/DescriptorPool.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>

#include <volk.h>

namespace Retina::Graphics {
  CDescriptorLayout::~CDescriptorLayout() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      vkDestroyDescriptorSetLayout(GetDescriptorPool().GetDevice().GetHandle(), _handle, nullptr);
      RETINA_GRAPHICS_INFO("Descriptor layout ({}) destroyed", GetDebugName());
    }
  }

  auto CDescriptorLayout::Make(
    const CDescriptorPool& descriptorPool,
    const SDescriptorLayoutCreateInfo& createInfo
  ) noexcept -> Core::CArcPtr<CDescriptorLayout> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CDescriptorLayout());
    auto descriptorLayoutBindings = std::vector<VkDescriptorSetLayoutBinding>();
    descriptorLayoutBindings.reserve(createInfo.Bindings.size());
    auto descriptorLayoutBindingFlags = std::vector<VkDescriptorBindingFlags>();
    descriptorLayoutBindingFlags.reserve(createInfo.Bindings.size());
    for (auto i = 0_u32; i < createInfo.Bindings.size(); ++i) {
      const auto& binding = createInfo.Bindings[i];
      auto descriptorLayoutBinding = VkDescriptorSetLayoutBinding();
      descriptorLayoutBinding.binding = i;
      descriptorLayoutBinding.descriptorType = AsEnumCounterpart(binding.Type);
      descriptorLayoutBinding.descriptorCount = binding.Count;
      descriptorLayoutBinding.stageFlags = AsEnumCounterpart(binding.Stages);
      descriptorLayoutBinding.pImmutableSamplers = nullptr;
      descriptorLayoutBindings.emplace_back(descriptorLayoutBinding);
      descriptorLayoutBindingFlags.emplace_back(AsEnumCounterpart(binding.Flags));
    }

    const auto& descriptorPoolInfo = descriptorPool.GetCreateInfo();
    auto descriptorLayoutBindingFlagInfo = VkDescriptorSetLayoutBindingFlagsCreateInfo();
    descriptorLayoutBindingFlagInfo.bindingCount = descriptorLayoutBindingFlags.size();
    descriptorLayoutBindingFlagInfo.pBindingFlags = descriptorLayoutBindingFlags.data();

    auto descriptorLayoutCreateInfo = VkDescriptorSetLayoutCreateInfo(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
    descriptorLayoutCreateInfo.pNext = &descriptorLayoutBindingFlagInfo;
    descriptorLayoutCreateInfo.flags = AsEnumCounterpart(createInfo.Flags);
    descriptorLayoutCreateInfo.bindingCount = descriptorLayoutBindings.size();
    descriptorLayoutCreateInfo.pBindings = descriptorLayoutBindings.data();

    if (Core::IsFlagEnabled(descriptorPoolInfo.Flags, EDescriptorPoolCreateFlag::E_UPDATE_AFTER_BIND)) {
      descriptorLayoutCreateInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    }

    auto descriptorLayoutHandle = VkDescriptorSetLayout();
    RETINA_GRAPHICS_VULKAN_CHECK(
      vkCreateDescriptorSetLayout(
        descriptorPool.GetDevice().GetHandle(),
        &descriptorLayoutCreateInfo,
        nullptr,
        &descriptorLayoutHandle
      )
    );

    self->_handle = descriptorLayoutHandle;
    self->_createInfo = createInfo;
    self->_descriptorPool = descriptorPool.ToArcPtr();
    self->SetDebugName(createInfo.Name);
    RETINA_GRAPHICS_INFO("Descriptor layout ({}) initialized", createInfo.Name);

    return self;
  }

  auto CDescriptorLayout::GetHandle() const noexcept -> VkDescriptorSetLayout {
    RETINA_PROFILE_SCOPED();
    return _handle;
  }

  auto CDescriptorLayout::GetCreateInfo() const noexcept -> const SDescriptorLayoutCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CDescriptorLayout::GetDescriptorPool() const noexcept -> const CDescriptorPool& {
    RETINA_PROFILE_SCOPED();
    return *_descriptorPool;
  }

  auto CDescriptorLayout::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CDescriptorLayout::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_SET_DEBUG_NAME(
      _descriptorPool->GetDevice().GetHandle(),
      _handle,
      VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
      name
    );
    _createInfo.Name = name;
  }

  auto CDescriptorLayout::GetBinding(uint32 index) const noexcept -> const SDescriptorLayoutBinding& {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Bindings[index];
  }

  auto CDescriptorLayout::FindBindingIndexFrom(EDescriptorType type) const noexcept -> std::optional<uint32> {
    RETINA_PROFILE_SCOPED();
    for (auto i = 0_u32; i < _createInfo.Bindings.size(); ++i) {
      if (_createInfo.Bindings[i].Type == type) {
        return i;
      }
    }
    return std::nullopt;
  }
}
