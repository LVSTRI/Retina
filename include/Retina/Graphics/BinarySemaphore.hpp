#pragma once

#include <Retina/Graphics/Semaphore.hpp>

namespace Retina::Graphics {
  class CBinarySemaphore : public ISemaphore {
  public:
    CBinarySemaphore(const CDevice& device) noexcept;
    ~CBinarySemaphore() noexcept override;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      const SBinarySemaphoreCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CBinarySemaphore>;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      uint32 count,
      const SBinarySemaphoreCreateInfo& createInfo
    ) noexcept -> std::vector<Core::CArcPtr<CBinarySemaphore>>;

    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SBinarySemaphoreCreateInfo&;
    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;

    auto SetDebugName(std::string_view name) noexcept -> void;

  private:
    SBinarySemaphoreCreateInfo _createInfo = {};
  };
}
