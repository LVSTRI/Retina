#include <Retina/Graphics/DeletionQueue.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/HostDeviceTimeline.hpp>

namespace Retina::Graphics {
  CDeletionQueue::CDeletionQueue(const CDevice& device) noexcept
    : _device(device) {}

  auto CDeletionQueue::Make(const CDevice& device) noexcept -> std::unique_ptr<CDeletionQueue> {
    RETINA_PROFILE_SCOPED();
    return std::make_unique<CDeletionQueue>(device);
  }

  auto CDeletionQueue::Enqueue(std::move_only_function<void()>&& packet) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    const auto& mainTimeline = _device->GetMainTimeline();
    _packets.emplace_back(mainTimeline.GetHostTimelineValue(), std::move(packet));
  }

  auto CDeletionQueue::Tick() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    const auto& mainTimeline = _device->GetMainTimeline();
    const auto currentTimelineValue = mainTimeline.GetDeviceTimelineValue();
    for (auto it = _packets.begin(); it != _packets.end();) {
      auto& [timelineValue, deletion] = *it;
      if (timelineValue < currentTimelineValue) {
        deletion();
        it = _packets.erase(it);
      } else {
        ++it;
      }
    }
  }

  auto CDeletionQueue::Flush() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    for (auto& packet : _packets) {
      packet.Deletion();
    }
    _packets.clear();
  }
}
