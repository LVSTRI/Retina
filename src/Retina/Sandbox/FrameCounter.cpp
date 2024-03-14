#include <Retina/Sandbox/FrameCounter.hpp>

namespace Retina::Sandbox {
  auto CFrameCounter::Make(uint64 current) noexcept -> CFrameCounter {
    RETINA_PROFILE_SCOPED();
    auto self = CFrameCounter();
    self._counter = current;
    return self;
  }

  auto CFrameCounter::GetCounterRelative() const noexcept -> uint64 {
    RETINA_PROFILE_SCOPED();
    RETINA_ASSERT_WITH(_parent, "Parent is null");
    return _parent->GetCounterAbsolute() - _counter;
  }

  auto CFrameCounter::GetCounterRelative(const CFrameCounter& other) const noexcept -> uint64 {
    RETINA_PROFILE_SCOPED();
    return other.GetCounterAbsolute() - _counter;
  }

  auto CFrameCounter::GetCounterAbsolute() const noexcept -> uint64 {
    RETINA_PROFILE_SCOPED();
    return _counter;
  }

  auto CFrameCounter::GetParent() const noexcept -> const CFrameCounter* {
    RETINA_PROFILE_SCOPED();
    return _parent;
  }

  auto CFrameCounter::Fork() noexcept -> CFrameCounter {
    RETINA_PROFILE_SCOPED();
    auto self = CFrameCounter();
    self._counter = _counter;
    self._parent = this;
    return self;
  }

  auto CFrameCounter::Tick() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    ++_counter;
  }

  auto CFrameCounter::Reset() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    if (_parent) {
      _counter = _parent->GetCounterAbsolute();
    } else {
      _counter = 0;
    }
  }
}
