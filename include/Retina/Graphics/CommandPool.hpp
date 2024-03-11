#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/CommandPoolInfo.hpp>
#include <Retina/Graphics/Forward.hpp>

namespace Retina::Graphics {
  class CCommandPool : public Core::IEnableIntrusiveReferenceCount<CCommandPool> {
  public:
    CCommandPool() noexcept = default;
    ~CCommandPool() noexcept;

    RETINA_NODISCARD static auto Make(
      const CQueue& queue,
      const SCommandPoolCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CCommandPool>;

    RETINA_NODISCARD static auto Make(
      const CQueue& queue,
      uint32 count,
      const SCommandPoolCreateInfo& createInfo
    ) noexcept -> std::vector<Core::CArcPtr<CCommandPool>>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkCommandPool;
    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SCommandPoolCreateInfo&;
    RETINA_NODISCARD auto GetQueue() const noexcept -> const CQueue&;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

    auto Reset() noexcept -> void;

  private:
    VkCommandPool _handle = {};

    SCommandPoolCreateInfo _createInfo = {};
    Core::CArcPtr<const CQueue> _queue;
  };
}
