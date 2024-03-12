#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/DeletionQueueInfo.hpp>

#include <memory>

namespace Retina::Graphics {
  class CDeletionQueue {
  public:
    CDeletionQueue() noexcept = default;
    ~CDeletionQueue() noexcept = default;
    RETINA_DELETE_COPY(CDeletionQueue);
    RETINA_DEFAULT_MOVE(CDeletionQueue);

    RETINA_NODISCARD static auto Make() noexcept -> std::unique_ptr<CDeletionQueue>;

    auto Enqueue(SDeletionQueuePacket&& packet) noexcept -> void;
    auto Tick() noexcept -> void;
    auto Flush() noexcept -> void;

  private:
    std::vector<SDeletionQueuePacket> _packets;
  };
}
