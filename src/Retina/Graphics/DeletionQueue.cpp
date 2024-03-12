#include <Retina/Graphics/DeletionQueue.hpp>

namespace Retina::Graphics {
  auto CDeletionQueue::Make() noexcept -> std::unique_ptr<CDeletionQueue> {
    RETINA_PROFILE_SCOPED();
    return std::make_unique<CDeletionQueue>();
  }

  auto CDeletionQueue::Enqueue(SDeletionQueuePacket&& packet) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _packets.emplace_back(std::move(packet));
  }

  auto CDeletionQueue::Tick() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    for (auto& packet : _packets) {
      if (--packet.TimeToLive == 0) {
        packet.Deletion();
      }
    }
    std::erase_if(_packets, [](const SDeletionQueuePacket& packet) {
      return packet.TimeToLive == 0;
    });
  }

  auto CDeletionQueue::Flush() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    for (auto& packet : _packets) {
      packet.Deletion();
    }
    _packets.clear();
  }
}
