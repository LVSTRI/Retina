#pragma once

#include <Retina/Graphics/DescriptorLayoutInfo.hpp>

#include <vulkan/vulkan.h>

namespace Retina::Graphics {
  class CDescriptorLayout : public Core::IEnableIntrusiveReferenceCount<CDescriptorLayout> {
  public:
    CDescriptorLayout() noexcept = default;
    ~CDescriptorLayout() noexcept;

    RETINA_NODISCARD static auto Make(
      const CDescriptorPool& descriptorPool,
      const SDescriptorLayoutCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CDescriptorLayout>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkDescriptorSetLayout;
    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SDescriptorLayoutCreateInfo&;
    RETINA_NODISCARD auto GetDescriptorPool() const noexcept -> const CDescriptorPool&;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

    RETINA_NODISCARD auto GetBinding(uint32 index) const noexcept -> const SDescriptorLayoutBinding&;

    RETINA_NODISCARD auto FindBindingIndexFrom(EDescriptorType type) const noexcept -> std::optional<uint32>;

  private:
    VkDescriptorSetLayout _handle = {};

    SDescriptorLayoutCreateInfo _createInfo = {};
    Core::CArcPtr<const CDescriptorPool> _descriptorPool;
  };
}
