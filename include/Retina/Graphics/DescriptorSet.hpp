#pragma once

#include <Retina/Graphics/DescriptorSetInfo.hpp>

#include <vulkan/vulkan.h>

#include <vector>
#include <span>

namespace Retina::Graphics {
  class CDescriptorSet : public Core::IEnableIntrusiveReferenceCount<CDescriptorSet> {
  public:
    CDescriptorSet() noexcept = default;
    ~CDescriptorSet() noexcept;

    RETINA_NODISCARD static auto Make(
      const CDescriptorLayout& layout,
      const SDescriptorSetCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CDescriptorSet>;

    RETINA_NODISCARD static auto Make(
      const CDescriptorLayout& layout,
      uint32 count,
      const SDescriptorSetCreateInfo& createInfo
    ) noexcept -> std::vector<Core::CArcPtr<CDescriptorSet>>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkDescriptorSet;
    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SDescriptorSetCreateInfo&;
    RETINA_NODISCARD auto GetLayout() const noexcept -> const CDescriptorLayout&;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

    auto Write(std::span<const SDescriptorWriteInfo> writes) noexcept -> void;

  private:
    VkDescriptorSet _handle = {};

    SDescriptorSetCreateInfo _createInfo = {};
    Core::CArcPtr<const CDescriptorLayout> _layout;
  };
}
