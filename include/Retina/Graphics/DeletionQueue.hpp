#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/DeletionQueueInfo.hpp>

#include <memory>

namespace Retina::Graphics {
  class CDeletionQueue {
  public:
    CDeletionQueue(const CDevice& device) noexcept;
    ~CDeletionQueue() noexcept = default;
    RETINA_DELETE_COPY(CDeletionQueue);
    RETINA_DEFAULT_MOVE(CDeletionQueue);

    RETINA_NODISCARD static auto Make(const CDevice& device) noexcept -> Core::CUniquePtr<CDeletionQueue>;

    auto Enqueue(std::move_only_function<void()>&& packet) noexcept -> void;
    auto Tick() noexcept -> void;
    auto Flush() noexcept -> void;

  private:
    std::vector<SDeletionQueuePacket> _packets;

    Core::CReferenceWrapper<const CDevice> _device;
  };
}
