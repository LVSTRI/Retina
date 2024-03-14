#pragma once

#include <Retina/Core/Core.hpp>

namespace Retina::Sandbox {
  class CFrameCounter {
  public:
    CFrameCounter() noexcept = default;
    ~CFrameCounter() noexcept = default;

    RETINA_NODISCARD static auto Make(uint64 current = 0) noexcept -> CFrameCounter;

    RETINA_NODISCARD auto GetCounterRelative() const noexcept -> uint64;
    RETINA_NODISCARD auto GetCounterRelative(const CFrameCounter& other) const noexcept -> uint64;
    RETINA_NODISCARD auto GetCounterAbsolute() const noexcept -> uint64;

    RETINA_NODISCARD auto GetParent() const noexcept -> const CFrameCounter*;

    RETINA_NODISCARD auto Fork() noexcept -> CFrameCounter;

    auto Tick() noexcept -> void;
    auto Reset() noexcept -> void;

  private:
    uint64 _counter = 0;

    CFrameCounter* _parent = nullptr;
  };
}
