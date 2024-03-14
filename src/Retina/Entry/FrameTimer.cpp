#pragma once

#include <Retina/Entry/FrameTimer.hpp>

namespace Retina::Entry {
  auto CFrameTimer::Make() noexcept -> CFrameTimer {
    RETINA_PROFILE_SCOPED();
    return {};
  }

  auto CFrameTimer::Now() noexcept -> TimePoint {
    RETINA_PROFILE_SCOPED();
    return Clock::now();
  }

  auto CFrameTimer::GetDeltaTime() const noexcept -> float32 {
    RETINA_PROFILE_SCOPED();
    return _deltaTime;
  }

  auto CFrameTimer::GetLastTime() const noexcept -> float32 {
    RETINA_PROFILE_SCOPED();
    return _lastTime;
  }

  auto CFrameTimer::Start() noexcept -> CFrameTimerProxy {
    RETINA_PROFILE_SCOPED();
    return { *this };
  }


  CFrameTimerProxy::CFrameTimerProxy(CFrameTimer& timer) noexcept
    : _timer(timer)
  {
    RETINA_PROFILE_SCOPED();
    _start = CFrameTimer::Now();
  }

  CFrameTimerProxy::~CFrameTimerProxy() noexcept {
    RETINA_PROFILE_SCOPED();
    auto end = CFrameTimer::Now();
    _timer->_lastTime = _timer->_deltaTime;
    _timer->_deltaTime = std::chrono::duration<float32, std::ratio<1>>(end - _start).count();
  }
}
