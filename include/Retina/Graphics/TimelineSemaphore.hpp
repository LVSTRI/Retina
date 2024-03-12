#pragma once

#include <Retina/Graphics/Semaphore.hpp>

namespace Retina::Graphics {
  class CTimelineSemaphore : public ISemaphore {
  public:
    CTimelineSemaphore(const CDevice& device) noexcept;
    ~CTimelineSemaphore() noexcept override;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      const STimelineSemaphoreCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CTimelineSemaphore>;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      uint32 count,
      const STimelineSemaphoreCreateInfo& createInfo
    ) noexcept -> std::vector<Core::CArcPtr<CTimelineSemaphore>>;

    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const STimelineSemaphoreCreateInfo&;
    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;

    auto SetDebugName(std::string_view name) noexcept -> void;

    RETINA_NODISCARD auto GetCounter() const noexcept -> uint64;
    auto Wait(uint64 value, uint64 timeout = -1_u64) const noexcept -> bool;
    auto Signal(uint64 value) const noexcept -> void;

  private:
    STimelineSemaphoreCreateInfo _createInfo = {};
  };
}
