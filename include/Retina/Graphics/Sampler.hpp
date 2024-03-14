#pragma once

#include <Retina/Graphics/SamplerInfo.hpp>

#include <vulkan/vulkan.h>

namespace Retina::Graphics {
  class CSampler : public Core::IEnableIntrusiveReferenceCount<CSampler> {
  public:
    CSampler(const CDevice& device) noexcept;
    ~CSampler() noexcept;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      const SSamplerCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CSampler>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkSampler;
    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SSamplerCreateInfo&;
    RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

    RETINA_NODISCARD auto GetDescriptor() const noexcept -> SImageDescriptor;

  private:
    VkSampler _handle = {};

    SSamplerCreateInfo _createInfo = {};
    Core::CReferenceWrapper<const CDevice> _device;
  };
}
