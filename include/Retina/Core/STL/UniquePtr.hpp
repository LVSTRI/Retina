#pragma once

#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Types.hpp>
#include <Retina/Core/Utility.hpp>

#include <compare>

namespace Retina::Core {
  template <typename T>
  class CUniquePtr {
  public:
    RETINA_INLINE constexpr CUniquePtr() noexcept = default;
    RETINA_INLINE constexpr ~CUniquePtr() noexcept;
    RETINA_DELETE_COPY(CUniquePtr, RETINA_INLINE constexpr);
    RETINA_DECLARE_MOVE(CUniquePtr, RETINA_INLINE constexpr);

    template <typename U>
    RETINA_INLINE constexpr CUniquePtr(U* ptr) noexcept;
    RETINA_INLINE constexpr CUniquePtr(std::nullptr_t) noexcept;

    template <typename U>
    RETINA_INLINE constexpr CUniquePtr(CUniquePtr<U>&& other) noexcept;
    template <typename U>
    RETINA_INLINE constexpr auto operator =(CUniquePtr<U>&& other) noexcept -> CUniquePtr&;

    RETINA_NODISCARD RETINA_INLINE constexpr auto Get() const noexcept -> T*;
    RETINA_INLINE constexpr auto Release() noexcept -> T*;
    RETINA_INLINE constexpr auto Reset() noexcept -> void;

    RETINA_NODISCARD RETINA_INLINE constexpr auto operator !() const noexcept -> bool;
    RETINA_NODISCARD RETINA_INLINE constexpr auto operator <=>(const CUniquePtr&) const noexcept -> std::strong_ordering = default;

    RETINA_NODISCARD RETINA_INLINE constexpr auto operator *() const noexcept -> T&;
    RETINA_NODISCARD RETINA_INLINE constexpr auto operator ->() const noexcept -> T*;

    RETINA_NODISCARD RETINA_INLINE constexpr operator bool() const noexcept;

    template <typename U>
    friend class CUniquePtr;

  private:
    T* _ptr = nullptr;
  };

  template <typename T>
  CUniquePtr(T*) -> CUniquePtr<T>;

  template <typename T>
  CUniquePtr(const T*) -> CUniquePtr<const T>;

  template <typename T>
  constexpr CUniquePtr<T>::~CUniquePtr() noexcept {
    Reset();
  }

  template <typename T>
  constexpr CUniquePtr<T>::CUniquePtr(CUniquePtr&& other) noexcept
    : _ptr(std::exchange(other._ptr, nullptr))
  {}

  template <typename T>
  constexpr auto CUniquePtr<T>::operator =(CUniquePtr&& other) noexcept -> CUniquePtr& {
    if (this == &other) {
      return *this;
    }
    return Reconstruct(*this, std::move(other));
  }

  template <typename T>
  template <typename U>
  constexpr CUniquePtr<T>::CUniquePtr(U* ptr) noexcept
    : _ptr(ptr)
  {}

  template <typename T>
  constexpr CUniquePtr<T>::CUniquePtr(std::nullptr_t) noexcept
    : _ptr(nullptr)
  {}

  template <typename T>
  template <typename U>
  constexpr CUniquePtr<T>::CUniquePtr(CUniquePtr<U>&& other) noexcept
    : _ptr(std::exchange(other._ptr, nullptr))
  {}

  template <typename T>
  template <typename U>
  constexpr auto CUniquePtr<T>::operator =(CUniquePtr<U>&& other) noexcept -> CUniquePtr& {
    return Reconstruct(*this, std::move(other));
  }

  template <typename T>
  constexpr auto CUniquePtr<T>::Get() const noexcept -> T* {
    return _ptr;
  }

  template <typename T>
  constexpr auto CUniquePtr<T>::Release() noexcept -> T* {
    return std::exchange(_ptr, nullptr);
  }

  template <typename T>
  constexpr auto CUniquePtr<T>::Reset() noexcept -> void {
    delete std::exchange(_ptr, nullptr);
  }

  template <typename T>
  constexpr auto CUniquePtr<T>::operator !() const noexcept -> bool {
    return !_ptr;
  }

  template <typename T>
  constexpr auto CUniquePtr<T>::operator *() const noexcept -> T& {
    return *_ptr;
  }

  template <typename T>
  constexpr auto CUniquePtr<T>::operator ->() const noexcept -> T* {
    return _ptr;
  }

  template <typename T>
  constexpr CUniquePtr<T>::operator bool() const noexcept {
    return _ptr;
  }

  template <typename T, typename... Args>
  RETINA_NODISCARD RETINA_INLINE constexpr auto MakeUnique(Args&&... args) -> CUniquePtr<T> {
    return CUniquePtr<T>(new T(std::forward<Args>(args)...));
  }
}
