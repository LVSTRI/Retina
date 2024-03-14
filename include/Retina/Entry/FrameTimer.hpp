#pragma once

#include <Retina/Core/Core.hpp>

#include <chrono>

#define RETINA_SCOPED_TIMER_IMPL(x, l) auto _timer##l = x.Start()
#define RETINA_SCOPED_TIMER(x) RETINA_SCOPED_TIMER_IMPL(x, __COUNTER__)

namespace Retina::Entry {
  class CFrameTimerProxy;

  class CFrameTimer {
  public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    CFrameTimer() noexcept = default;
    ~CFrameTimer() noexcept = default;

    RETINA_NODISCARD static auto Make() noexcept -> CFrameTimer;

    RETINA_NODISCARD static auto Now() noexcept -> TimePoint;

    RETINA_NODISCARD auto GetDeltaTime() const noexcept -> float32;
    RETINA_NODISCARD auto GetLastTime() const noexcept -> float32;

    RETINA_NODISCARD auto Start() noexcept -> CFrameTimerProxy;

  private:
    friend class CFrameTimerProxy;

    Clock _clock;
    float32 _deltaTime = 0.0f;
    float32 _lastTime = 0.0f;
  };

  class CFrameTimerProxy {
  public:
    CFrameTimerProxy(CFrameTimer& timer) noexcept;
    ~CFrameTimerProxy() noexcept;
    RETINA_DELETE_COPY(CFrameTimerProxy);
    RETINA_DEFAULT_MOVE(CFrameTimerProxy);

  private:
    CFrameTimer::TimePoint _start;

    Core::CReferenceWrapper<CFrameTimer> _timer;
  };
}
