#include <Retina/Graphics/DescriptorLayout.hpp>
#include <Retina/Graphics/DescriptorPool.hpp>
#include <Retina/Graphics/DescriptorSet.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/ImageView.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>

#include <volk.h>

namespace Retina::Graphics {
  namespace Details {
    RETINA_NODISCARD RETINA_INLINE auto GetNativeImageDescriptors(
      std::span<const SImageDescriptor> descriptors
    ) noexcept -> std::vector<VkDescriptorImageInfo> {
      RETINA_PROFILE_SCOPED();
      auto result = std::vector<VkDescriptorImageInfo>();
      result.reserve(descriptors.size());
      for (const auto& [view, layout] : descriptors) {
        result.emplace_back(VK_NULL_HANDLE, view->GetHandle(), AsEnumCounterpart(layout));
      }
      return result;
    }

    RETINA_NODISCARD RETINA_INLINE auto GetNativeBufferDescriptors(
      std::span<const SBufferDescriptor> descriptors
    ) noexcept -> std::vector<VkDescriptorBufferInfo> {
      RETINA_PROFILE_SCOPED();
      // TODO: Implement
      RETINA_GRAPHICS_PANIC_WITH("Buffer descriptors are not yet implemented");
      return {};
    }
  }

  CDescriptorSet::~CDescriptorSet() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      const auto& pool = GetLayout().GetDescriptorPool();
      const auto& poolInfo = pool.GetCreateInfo();
      const auto& device = pool.GetDevice();
      if (Core::IsFlagEnabled(poolInfo.Flags, EDescriptorPoolCreateFlag::E_FREE_DESCRIPTOR_SET)) {
        RETINA_GRAPHICS_VULKAN_CHECK(vkFreeDescriptorSets(device.GetHandle(), pool.GetHandle(), 1, &_handle));
        RETINA_GRAPHICS_INFO("Descriptor set ({}) destroyed", GetDebugName());
      }
    }
  }

  auto CDescriptorSet::Make(
    const CDescriptorLayout& layout,
    const SDescriptorSetCreateInfo& createInfo
  ) noexcept -> Core::CArcPtr<CDescriptorSet> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CDescriptorSet());
    const auto& descriptorPool = layout.GetDescriptorPool();
    const auto& device = descriptorPool.GetDevice();

    const auto layoutHandle = layout.GetHandle();
    auto descriptorSetAllocateInfo = VkDescriptorSetAllocateInfo(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
    descriptorSetAllocateInfo.descriptorPool = descriptorPool.GetHandle();
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &layoutHandle;

    auto descriptorSetHandle = VkDescriptorSet();
    RETINA_GRAPHICS_VULKAN_CHECK(vkAllocateDescriptorSets(device.GetHandle(), &descriptorSetAllocateInfo, &descriptorSetHandle));
    RETINA_GRAPHICS_INFO("Descriptor set ({}) initialized with:", createInfo.Name);
    RETINA_GRAPHICS_INFO("  - Descriptor pool: {}", descriptorPool.GetDebugName());
    RETINA_GRAPHICS_INFO("  - Descriptor layout: {}", layout.GetDebugName());

    self->_handle = descriptorSetHandle;
    self->_createInfo = createInfo;
    self->_layout = layout.ToArcPtr();
    return self;
  }

  auto CDescriptorSet::Make(
    const CDescriptorLayout& layout,
    uint32 count,
    const SDescriptorSetCreateInfo& createInfo
  ) noexcept -> std::vector<Core::CArcPtr<CDescriptorSet>> {
    RETINA_PROFILE_SCOPED();
    auto descriptorSets = std::vector<Core::CArcPtr<CDescriptorSet>>();
    descriptorSets.reserve(count);
    for (auto i = 0_u32; i < count; ++i) {
      auto currentCreateInfo = createInfo;
      currentCreateInfo.Name += std::to_string(i);
      descriptorSets.emplace_back(Make(layout, createInfo));
    }
    return descriptorSets;
  }

  auto CDescriptorSet::GetHandle() const noexcept -> VkDescriptorSet {
    RETINA_PROFILE_SCOPED();
    return _handle;
  }

  auto CDescriptorSet::GetCreateInfo() const noexcept -> const SDescriptorSetCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CDescriptorSet::GetLayout() const noexcept -> const CDescriptorLayout& {
    RETINA_PROFILE_SCOPED();
    return *_layout;
  }

  auto CDescriptorSet::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CDescriptorSet::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    const auto& device = GetLayout().GetDescriptorPool().GetDevice();
    RETINA_GRAPHICS_SET_DEBUG_NAME(device.GetHandle(), _handle, VK_OBJECT_TYPE_DESCRIPTOR_SET, name);
    _createInfo.Name = name;
  }

  auto CDescriptorSet::Write(std::span<const SDescriptorWriteInfo> writes) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    auto descriptorWrites = std::vector<VkWriteDescriptorSet>();
    auto descriptorBufferInfos = std::vector<std::vector<VkDescriptorBufferInfo>>();
    auto descriptorImageInfos = std::vector<std::vector<VkDescriptorImageInfo>>();

    descriptorWrites.reserve(writes.size());
    descriptorBufferInfos.reserve(writes.size());
    descriptorImageInfos.reserve(writes.size());
    for (const auto& write : writes) {
      auto descriptorWrite = VkWriteDescriptorSet(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
      descriptorWrite.dstSet = _handle;
      descriptorWrite.dstBinding = write.Binding;
      descriptorWrite.dstArrayElement = write.Slot;
      descriptorWrite.descriptorType = AsEnumCounterpart(write.Type);

      if (write.Binding == -1_u32) {
        descriptorWrite.dstBinding = *GetLayout().FindBindingIndexFrom(write.Type);
      }

      switch (write.Type) {
        case EDescriptorType::E_SAMPLER: RETINA_FALLTHROUGH;
        case EDescriptorType::E_COMBINED_IMAGE_SAMPLER: RETINA_FALLTHROUGH;
        case EDescriptorType::E_SAMPLED_IMAGE: RETINA_FALLTHROUGH;
        case EDescriptorType::E_STORAGE_IMAGE: RETINA_FALLTHROUGH;
        case EDescriptorType::E_INPUT_ATTACHMENT: {
          auto imageDescriptors = Details::GetNativeImageDescriptors(std::get<std::vector<SImageDescriptor>>(write.Descriptors));
          descriptorWrite.descriptorCount = imageDescriptors.size();
          descriptorWrite.pImageInfo = imageDescriptors.data();

          descriptorWrites.emplace_back(descriptorWrite);
          descriptorImageInfos.emplace_back(std::move(imageDescriptors));
          break;
        }

        case EDescriptorType::E_UNIFORM_TEXEL_BUFFER: RETINA_FALLTHROUGH;
        case EDescriptorType::E_STORAGE_TEXEL_BUFFER:
          RETINA_GRAPHICS_PANIC_WITH("Texel buffers are not supported");

        case EDescriptorType::E_UNIFORM_BUFFER: RETINA_FALLTHROUGH;
        case EDescriptorType::E_STORAGE_BUFFER: RETINA_FALLTHROUGH;
        case EDescriptorType::E_UNIFORM_BUFFER_DYNAMIC: RETINA_FALLTHROUGH;
        case EDescriptorType::E_STORAGE_BUFFER_DYNAMIC: {
          auto bufferDescriptors = Details::GetNativeBufferDescriptors(std::get<std::vector<SBufferDescriptor>>(write.Descriptors));
          descriptorWrite.descriptorCount = bufferDescriptors.size();
          descriptorWrite.pBufferInfo = bufferDescriptors.data();

          descriptorWrites.emplace_back(descriptorWrite);
          descriptorBufferInfos.emplace_back(std::move(bufferDescriptors));
          break;
        }

        default:
          RETINA_GRAPHICS_PANIC_WITH("Unsupported descriptor type");
      }
    }

    if (!descriptorWrites.empty()) {
      const auto& device = GetLayout().GetDescriptorPool().GetDevice();
      vkUpdateDescriptorSets(device.GetHandle(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
  }
}
