#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/HostDeviceTimeline.hpp>
#include <Retina/Graphics/TimelineSemaphore.hpp>

namespace Retina::Graphics {
  auto CHostDeviceTimeline::Make(
    const CDevice& device,
    uint64 maxTimelineDifference
  ) noexcept -> CHostDeviceTimeline {
    RETINA_PROFILE_SCOPED();
    auto self = CHostDeviceTimeline();
    self._maxTimelineDifference = maxTimelineDifference;
    self._hostTimelineValue = 0;
    self._deviceTimeline = CTimelineSemaphore::Make(device, {
      .Name = "HostDeviceTimeline_DeviceTimeline",
      .Value = 0,
    });
    self._device = device.ToArcPtr();
    return self;
  }

  auto CHostDeviceTimeline::GetMaxTimelineDifference() const noexcept -> uint64 {
    RETINA_PROFILE_SCOPED();
    return _maxTimelineDifference;
  }

  auto CHostDeviceTimeline::GetHostTimelineValue() const noexcept -> uint64 {
    RETINA_PROFILE_SCOPED();
    return _hostTimelineValue;
  }

  auto CHostDeviceTimeline::GetDeviceTimeline() const noexcept -> const CTimelineSemaphore& {
    RETINA_PROFILE_SCOPED();
    return *_deviceTimeline;
  }

  auto CHostDeviceTimeline::GetDevice() const noexcept -> const CDevice& {
    RETINA_PROFILE_SCOPED();
    return *_device;
  }

  auto CHostDeviceTimeline::GetDeviceTimelineValue() const noexcept -> uint64 {
    RETINA_PROFILE_SCOPED();
    return _deviceTimeline->GetCounter();
  }

  auto CHostDeviceTimeline::WaitForNextHostTimelineValue() noexcept -> uint64 {
    RETINA_PROFILE_SCOPED();
    const auto target = static_cast<int64>(_hostTimelineValue) - static_cast<int64>(_maxTimelineDifference);
    const auto next = std::max<int64>(target + 1, 0);
    _deviceTimeline->Wait(next);
    return _hostTimelineValue++;
  }
}

