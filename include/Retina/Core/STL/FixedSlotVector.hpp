#pragma once

#include <Retina/Core/STL/FixedVector.hpp>
#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Types.hpp>

#include <vector>

namespace Retina::Core {
  template <typename T, usize N, typename S = usize>
  class CFixedSlotVector {
  public:
    constexpr CFixedSlotVector() noexcept = default;
    constexpr ~CFixedSlotVector() noexcept = default;
    RETINA_DEFAULT_COPY_MOVE(CFixedSlotVector, constexpr);

    RETINA_NODISCARD constexpr static auto Make() noexcept -> CFixedSlotVector;

    template <typename... Args>
    RETINA_NODISCARD constexpr auto Insert(Args&&... args) noexcept -> S;
    RETINA_NODISCARD constexpr auto Insert(const T& value) noexcept -> S;
    RETINA_NODISCARD constexpr auto Insert(T&& value) noexcept -> S;

    constexpr auto Remove(const S& slot) noexcept -> void;

    RETINA_NODISCARD constexpr auto Get(const S& slot) noexcept -> T&;
    RETINA_NODISCARD constexpr auto Get(const S& slot) const noexcept -> const T&;

    RETINA_NODISCARD constexpr auto GetSize() const noexcept -> usize;

    RETINA_NODISCARD constexpr auto IsEmpty() const noexcept -> bool;
    RETINA_NODISCARD constexpr auto IsFull() const noexcept -> bool;

  private:
    RETINA_NODISCARD constexpr auto GetFreeSlot() noexcept -> S;

  private:
    CFixedVector<T, N> _storage;
    CFixedVector<S, N> _free;
  };

  template <typename T, usize N, typename S>
  constexpr auto CFixedSlotVector<T, N, S>::Make() noexcept -> CFixedSlotVector {
    return {};
  }

  template <typename T, usize N, typename S>
  template <typename... Args>
  constexpr auto CFixedSlotVector<T, N, S>::Insert(Args&&... args) noexcept -> S {
    const auto slot = GetFreeSlot();
    _storage[slot] = T(std::forward<Args>(args)...);
    return slot;
  }

  template <typename T, usize N, typename S>
  constexpr auto CFixedSlotVector<T, N, S>::Insert(const T& value) noexcept -> S {
    const auto slot = GetFreeSlot();
    _storage[slot] = value;
    return slot;
  }

  template <typename T, usize N, typename S>
  constexpr auto CFixedSlotVector<T, N, S>::Insert(T&& value) noexcept -> S {
    const auto slot = GetFreeSlot();
    _storage[slot] = std::move(value);
    return slot;
  }

  template <typename T, usize N, typename S>
  constexpr auto CFixedSlotVector<T, N, S>::Remove(const S& slot) noexcept -> void {
    _free.PushBack(slot);
    _storage[slot] = {};
  }

  template <typename T, usize N, typename S>
  constexpr auto CFixedSlotVector<T, N, S>::Get(const S& slot) noexcept -> T& {
    return _storage[slot];
  }

  template <typename T, usize N, typename S>
  constexpr auto CFixedSlotVector<T, N, S>::Get(const S& slot) const noexcept -> const T& {
    return _storage[slot];
  }

  template <typename T, usize N, typename S>
  constexpr auto CFixedSlotVector<T, N, S>::GetSize() const noexcept -> usize {
    return _storage.GetSize();
  }

  template <typename T, usize N, typename S>
  constexpr auto CFixedSlotVector<T, N, S>::IsEmpty() const noexcept -> bool {
    return _storage.IsEmpty();
  }

  template <typename T, usize N, typename S>
  constexpr auto CFixedSlotVector<T, N, S>::IsFull() const noexcept -> bool {
    return _storage.IsFull();
  }

  template <typename T, usize N, typename S>
  constexpr auto CFixedSlotVector<T, N, S>::GetFreeSlot() noexcept -> S {
    RETINA_ASSERT_WITH(!IsFull(), "No free slots remaining");
    if (_free.IsEmpty()) {
      const auto slot = _storage.GetSize();
      _storage.EmplaceBack();
      return slot;
    }
    return _free.PopBack();
  }
}
