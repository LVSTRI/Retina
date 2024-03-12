#pragma once

#include <Retina/Graphics/DescriptorPoolInfo.hpp>

#include <vulkan/vulkan.h>

namespace Retina::Graphics {
  class CDescriptorPool : public Core::IEnableIntrusiveReferenceCount<CDescriptorPool> {
  public:
    CDescriptorPool() noexcept = default;
    ~CDescriptorPool() noexcept;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      const SDescriptorPoolCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CDescriptorPool>;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      uint32 count,
      const SDescriptorPoolCreateInfo& createInfo
    ) noexcept -> std::vector<Core::CArcPtr<CDescriptorPool>>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkDescriptorPool;
    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SDescriptorPoolCreateInfo&;
    RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

    auto Reset() noexcept -> void;

  private:
    VkDescriptorPool _handle = {};

    SDescriptorPoolCreateInfo _createInfo = {};
    Core::CArcPtr<const CDevice> _device;
  };
}
