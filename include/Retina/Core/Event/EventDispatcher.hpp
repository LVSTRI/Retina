
#pragma once

#include <Retina/Core/STL/FlatHashMap.hpp>

#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Traits.hpp>
#include <Retina/Core/Types.hpp>

#include <functional>
#include <vector>
#include <tuple>

namespace Retina::Core {
  template <typename... Es>
  class CEventDispatcher {
  public:
    template <typename E>
    using EventCallbackFunction = std::move_only_function<bool(const E&)>;

    template <typename E>
    using EventCallbackList = FlatHashMap<uintptr, EventCallbackFunction<E>>;

    constexpr CEventDispatcher() noexcept = default;
    constexpr ~CEventDispatcher() noexcept = default;
    RETINA_DEFAULT_COPY_MOVE(CEventDispatcher, constexpr);

    // Out of line definitions crash clang 18.1.1
    RETINA_NODISCARD constexpr static auto Make() noexcept -> CEventDispatcher {
      return {};
    }

    template <typename T, typename... Args>
      requires (TContains<T, Es...>)
    constexpr auto Fire(Args&&... args) noexcept -> void {
      const auto event = T(std::forward<Args>(args)...);
      auto& callbacks = std::get<EventCallbackList<T>>(_storage);
      for (auto& [_, callback] : callbacks) {
        if (!callback(event)) {
          break;
        }
      }
    }

    template <typename E, typename C>
      requires (TContains<E, Es...>)
    constexpr auto Attach(C* receiver, auto (C::* func)(const E&) noexcept -> bool) noexcept -> void {
      auto& callbacks = std::get<EventCallbackList<E>>(_storage);
      const auto id = reinterpret_cast<uintptr>(receiver);
      callbacks[id] = [receiver, func](const E& event) noexcept -> bool {
        return (receiver->*func)(event);
      };
    }

    template <typename E, typename C>
      requires (TContains<E, Es...>)
    constexpr auto Attach(C* receiver, auto (C::* func)(const E&) const noexcept -> bool) noexcept -> void {
      auto& callbacks = std::get<EventCallbackList<E>>(_storage);
      const auto id = reinterpret_cast<uintptr>(receiver);
      callbacks[id] = [receiver, func](const E& event) noexcept -> bool {
        return (receiver->*func)(event);
      };
    }

    template <typename E, typename C>
      requires (TContains<E, Es...>)
    constexpr auto Detach(C* receiver) noexcept -> void {
      auto& callbacks = std::get<EventCallbackList<E>>(_storage);
      const auto id = reinterpret_cast<uintptr>(receiver);
      callbacks.erase(id);
    }

  private:
    std::tuple<EventCallbackList<Es>...> _storage;
  };
}
