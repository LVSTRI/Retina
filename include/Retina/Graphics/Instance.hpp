#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/InstanceInfo.hpp>

#include <vulkan/vulkan.h>

namespace Retina::Graphics {
  class CInstance : public Core::IEnableIntrusiveReferenceCount<CInstance> {
  public:
    CInstance() noexcept = default;
    ~CInstance() noexcept;

    RETINA_NODISCARD static auto Make(const SInstanceCreateInfo& createInfo) noexcept -> Core::CArcPtr<CInstance>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkInstance;
    RETINA_NODISCARD auto GetVersion() const noexcept -> uint32;
    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SInstanceCreateInfo&;

    RETINA_NODISCARD auto IsFeatureEnabled(bool SInstanceFeature::* feature) const noexcept -> bool;

    RETINA_NODISCARD auto LoadFunction(std::string_view name) const noexcept -> PFN_vkVoidFunction;

  private:
    VkInstance _handle = {};
    uint32 _version = 0;

    SInstanceCreateInfo _createInfo = {};
  };
}
