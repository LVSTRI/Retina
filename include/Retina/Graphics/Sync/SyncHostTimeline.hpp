#pragma once

#include <Retina/Core/Core.hpp>

#include <spdlog/spdlog.h>

namespace Retina {
    class CSyncHostDeviceTimeline : public IEnableIntrusiveReferenceCount<CSyncHostDeviceTimeline> {
    public:
        using Self = CSyncHostDeviceTimeline;

        CSyncHostDeviceTimeline() noexcept = default;
        ~CSyncHostDeviceTimeline() noexcept = default;

        RETINA_NODISCARD static auto Make(const CDevice& device, uint64 maxTimelineDifference = 2) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetHostTimelineValue() const noexcept -> uint64;
        RETINA_NODISCARD auto GetDeviceTimelineSemaphore() const noexcept -> const CTimelineSemaphore&;

        RETINA_NODISCARD auto GetDeviceTimelineValue() const noexcept -> uint64;

        RETINA_NODISCARD auto WaitForNextTimelineValue() noexcept -> uint64;
        RETINA_NODISCARD auto GetNextSignalTimelineValue() const noexcept -> uint64;
        auto Reset() noexcept -> void;

    private:
        uint64 _hostTimelineValue = 0;
        CArcPtr<CTimelineSemaphore> _deviceTimelineSemaphore;

        uint64 _maxTimelineDifference = 0;
        std::shared_ptr<spdlog::logger> _logger;
        CArcPtr<const CDevice> _device;
    };
}
