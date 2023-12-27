#include <Retina/Graphics/Sync/SyncHostTimeline.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/TimelineSemaphore.hpp>

#include <spdlog/sinks/stdout_color_sinks.h>

namespace Retina {
    auto CSyncHostDeviceTimeline::Make(const CDevice& device, uint64 maxTimelineDifference) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto timeline = CArcPtr(new Self());
        auto logger = spdlog::stdout_color_mt("SyncHostTimeline");
        timeline->_hostTimelineValue = 0;
        timeline->_deviceTimelineSemaphore = CTimelineSemaphore::Make(device, {
            .Name = "SyncHostTimeline_DeviceTimelineSemaphore",
            .Counter = 0,
        });
        timeline->_maxTimelineDifference = maxTimelineDifference;
        timeline->_logger = std::move(logger);
        timeline->_device = device.ToArcPtr();
        return timeline;
    }

    auto CSyncHostDeviceTimeline::GetHostTimelineValue() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        return _hostTimelineValue;
    }

    auto CSyncHostDeviceTimeline::GetDeviceTimelineSemaphore() const noexcept -> const CTimelineSemaphore& {
        RETINA_PROFILE_SCOPED();
        return *_deviceTimelineSemaphore;
    }

    auto CSyncHostDeviceTimeline::GetDeviceTimelineValue() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        return _deviceTimelineSemaphore->GetCounter();
    }

    auto CSyncHostDeviceTimeline::WaitForNextTimelineValue() noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        const auto target = static_cast<int64>(_hostTimelineValue) - static_cast<int64>(_maxTimelineDifference);
        _deviceTimelineSemaphore->Wait(std::max<int64>(target + 1, 0));
        return _hostTimelineValue++ % _maxTimelineDifference;
    }

    auto CSyncHostDeviceTimeline::GetNextSignalTimelineValue() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        return _hostTimelineValue;
    }

    auto CSyncHostDeviceTimeline::Reset() noexcept -> void {
        RETINA_PROFILE_SCOPED();
        _hostTimelineValue = 0;
        _deviceTimelineSemaphore = CTimelineSemaphore::Make(*_device, _deviceTimelineSemaphore->GetCreateInfo());
    }
}
