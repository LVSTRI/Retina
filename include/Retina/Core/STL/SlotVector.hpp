#pragma once

#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Types.hpp>

#include <vector>

namespace Retina::Core {
  template <typename T, typename S = usize>
  class CSlotVector {
  public:
    constexpr CSlotVector() noexcept = default;
    constexpr ~CSlotVector() noexcept = default;
    RETINA_DEFAULT_COPY_MOVE(CSlotVector, constexpr);

    RETINA_NODISCARD constexpr static auto Make() noexcept -> CSlotVector;
    RETINA_NODISCARD constexpr static auto Make(usize capacity) noexcept -> CSlotVector;

    template <typename... Args>
    RETINA_NODISCARD constexpr auto Insert(Args&&... args) noexcept -> S;
    RETINA_NODISCARD constexpr auto Insert(const T& value) noexcept -> S;
    RETINA_NODISCARD constexpr auto Insert(T&& value) noexcept -> S;

    constexpr auto Remove(const S& slot) noexcept -> void;

    RETINA_NODISCARD constexpr auto Get(const S& slot) noexcept -> T&;
    RETINA_NODISCARD constexpr auto Get(const S& slot) const noexcept -> const T&;

    RETINA_NODISCARD constexpr auto GetSize() const noexcept -> usize;

    RETINA_NODISCARD constexpr auto IsEmpty() const noexcept -> bool;

  private:
    RETINA_NODISCARD constexpr auto GetFreeSlot() noexcept -> S;

    std::vector<T> _storage;
    std::vector<S> _free;
  };

  template <typename T, typename S>
  constexpr auto CSlotVector<T, S>::Make() noexcept -> CSlotVector {
    return Make(1024);
  }

  template <typename T, typename S>
  constexpr auto CSlotVector<T, S>::Make(usize capacity) noexcept -> CSlotVector {
    auto self = CSlotVector();
    self._storage.reserve(capacity);
    self._free.reserve(capacity);
    return self;
  }

  template <typename T, typename S>
  template <typename... Args>
  constexpr auto CSlotVector<T, S>::Insert(Args&&... args) noexcept -> S {
    const auto slot = GetFreeSlot();
    _storage[slot] = T(std::forward<Args>(args)...);
    return slot;
  }

  template <typename T, typename S>
  constexpr auto CSlotVector<T, S>::Insert(const T& value) noexcept -> S {
    const auto slot = GetFreeSlot();
    _storage[slot] = value;
    return slot;
  }

  template <typename T, typename S>
  constexpr auto CSlotVector<T, S>::Insert(T&& value) noexcept -> S {
    const auto slot = GetFreeSlot();
    _storage[slot] = std::move(value);
    return slot;
  }

  template <typename T, typename S>
  constexpr auto CSlotVector<T, S>::Remove(const S& slot) noexcept -> void {
    _storage[slot] = T();
    _free.emplace_back(slot);
  }

  template <typename T, typename S>
  constexpr auto CSlotVector<T, S>::Get(const S& slot) noexcept -> T& {
    return _storage[slot];
  }

  template <typename T, typename S>
  constexpr auto CSlotVector<T, S>::Get(const S& slot) const noexcept -> const T& {
    return _storage[slot];
  }

  template <typename T, typename S>
  constexpr auto CSlotVector<T, S>::GetSize() const noexcept -> usize {
    return _storage.size();
  }

  template <typename T, typename S>
  constexpr auto CSlotVector<T, S>::IsEmpty() const noexcept -> bool {
    return _storage.empty();
  }

  template <typename T, typename S>
  constexpr auto CSlotVector<T, S>::GetFreeSlot() noexcept -> S {
    if (_free.empty()) {
      const auto slot = _storage.size();
      _storage.emplace_back();
      return slot;
    }

    const auto slot = _free.back();
    _free.pop_back();
    return slot;
  }
}
