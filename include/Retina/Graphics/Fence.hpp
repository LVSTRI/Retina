#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/FenceInfo.hpp>
#include <Retina/Graphics/Forward.hpp>

#include <vulkan/vulkan.h>

namespace Retina::Graphics {
  class CFence : public Core::IEnableIntrusiveReferenceCount<CFence> {
  public:
    CFence() noexcept = default;
    ~CFence() noexcept;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      const SFenceCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CFence>;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      uint32 count,
      const SFenceCreateInfo& createInfo
    ) noexcept -> std::vector<Core::CArcPtr<CFence>>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkFence;
    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SFenceCreateInfo&;
    RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;

    auto SetDebugName(std::string_view name) noexcept -> void;

    RETINA_NODISCARD auto IsReady() const noexcept -> bool;

    auto Reset() noexcept -> void;
    auto Wait(uint64 timeout = -1_u64) noexcept -> void;

  private:
    VkFence _handle = {};

    SFenceCreateInfo _createInfo = {};
    Core::CArcPtr<const CDevice> _device;
  };
}
