#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Forward.hpp>

#include <memory>

namespace Retina::Graphics {
  class CHostDeviceTimeline {
  public:
    CHostDeviceTimeline(const CDevice& device) noexcept;
    ~CHostDeviceTimeline() noexcept;
    RETINA_DELETE_COPY(CHostDeviceTimeline);
    RETINA_DEFAULT_MOVE(CHostDeviceTimeline);

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      uint64 maxTimelineDifference = 2
    ) noexcept -> Core::CUniquePtr<CHostDeviceTimeline>;

    RETINA_NODISCARD auto GetMaxTimelineDifference() const noexcept -> uint64;
    RETINA_NODISCARD auto GetHostTimelineValue() const noexcept -> uint64;
    RETINA_NODISCARD auto GetDeviceTimeline() const noexcept -> const CTimelineSemaphore&;
    RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

    RETINA_NODISCARD auto GetDeviceTimelineValue() const noexcept -> uint64;
    RETINA_NODISCARD auto GetCurrentTimelineDifference() const noexcept -> uint64;

    RETINA_NODISCARD auto GetNextHostTimelineValue() noexcept -> uint64;
    RETINA_NODISCARD auto WaitForNextHostTimelineValue() const noexcept -> uint64;

  private:
    uint64 _maxTimelineDifference = 0;
    uint64 _hostTimelineValue = 0;
    Core::CArcPtr<const CTimelineSemaphore> _deviceTimeline;

    Core::CReferenceWrapper<const CDevice> _device;
  };
}
