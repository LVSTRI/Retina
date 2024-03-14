#pragma once

#include <utility>

#define RETINA_DEFER(f) const auto _defer = ::Retina::Core::CDefer(f)
#define RETINA_STATIC_DEFER(f) const static auto _defer = ::Retina::Core::CDefer(f)

namespace Retina::Core {
  template <typename F>
  class CDefer {
  public:
    constexpr CDefer(F&& function) noexcept;
    constexpr ~CDefer() noexcept;

  private:
    F _function;
  };

  template <typename F>
  constexpr CDefer<F>::CDefer(F&& function) noexcept
    : _function(std::forward<F>(function))
  {}

  template <typename F>
  constexpr CDefer<F>::~CDefer() noexcept {
    std::invoke(std::move(_function));
  }
}
