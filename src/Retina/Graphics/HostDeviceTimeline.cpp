#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/HostDeviceTimeline.hpp>
#include <Retina/Graphics/TimelineSemaphore.hpp>

namespace Retina::Graphics {
  CHostDeviceTimeline::CHostDeviceTimeline(const CDevice& device) noexcept
    : _device(device)
  {
    RETINA_PROFILE_SCOPED();
  }

  CHostDeviceTimeline::~CHostDeviceTimeline() noexcept = default;

  auto CHostDeviceTimeline::Make(
    const CDevice& device,
    uint64 maxTimelineDifference
  ) noexcept -> std::unique_ptr<CHostDeviceTimeline> {
    RETINA_PROFILE_SCOPED();
    auto self = std::make_unique<CHostDeviceTimeline>(device);
    self->_maxTimelineDifference = maxTimelineDifference;
    self->_hostTimelineValue = 0;
    self->_deviceTimeline = CTimelineSemaphore::Make(device, {
      .Name = "HostDeviceTimeline_DeviceTimeline",
      .Value = 0,
    });
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

  auto CHostDeviceTimeline::GetCurrentTimelineDifference() const noexcept -> uint64 {
    RETINA_PROFILE_SCOPED();
    return GetHostTimelineValue() - GetDeviceTimelineValue();
  }

  auto CHostDeviceTimeline::GetNextHostTimelineValue() noexcept -> uint64 {
    RETINA_PROFILE_SCOPED();
    return ++_hostTimelineValue;
  }

  auto CHostDeviceTimeline::WaitForNextHostTimelineValue() const noexcept -> uint64 {
    RETINA_PROFILE_SCOPED();
    const auto target = static_cast<int64>(_hostTimelineValue) - static_cast<int64>(_maxTimelineDifference);
    const auto next = std::max<int64>(target + 1, 0);
    _deviceTimeline->Wait(next);
    return _hostTimelineValue;
  }
}

