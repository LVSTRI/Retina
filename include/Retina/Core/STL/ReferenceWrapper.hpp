#pragma once

#include <Retina/Core/Macros.hpp>

#include <utility>
#include <compare>

namespace Retina::Core {
  template <typename T>
  class CReferenceWrapper {
  public:
    constexpr CReferenceWrapper() noexcept = default;
    constexpr ~CReferenceWrapper() noexcept = default;
    RETINA_DEFAULT_COPY_MOVE(CReferenceWrapper, constexpr);

    template <typename U>
    RETINA_INLINE constexpr CReferenceWrapper(U&& ref) noexcept;

    RETINA_NODISCARD RETINA_INLINE constexpr auto Get() const noexcept -> T&;

    RETINA_NODISCARD RETINA_INLINE constexpr auto operator *() const noexcept -> T&;
    RETINA_NODISCARD RETINA_INLINE constexpr auto operator ->() const noexcept -> T*;

    RETINA_NODISCARD RETINA_INLINE constexpr operator T&() const noexcept;

    template <typename... Args>
    RETINA_NODISCARD RETINA_INLINE constexpr auto operator ()(Args&&... args) const noexcept -> decltype(auto);

    RETINA_NODISCARD RETINA_INLINE constexpr auto operator <=>(const CReferenceWrapper&) const noexcept -> std::strong_ordering = default;

  private:
    T* _ptr = nullptr;
  };

  template <typename T>
  template <typename U>
  constexpr CReferenceWrapper<T>::CReferenceWrapper(U&& ref) noexcept {
    T& unwrapped = std::forward<U>(ref);
    _ptr = std::addressof(unwrapped);
  }

  template <typename T>
  constexpr auto CReferenceWrapper<T>::Get() const noexcept -> T& {
    return *_ptr;
  }

  template <typename T>
  constexpr auto CReferenceWrapper<T>::operator *() const noexcept -> T& {
    return *_ptr;
  }

  template <typename T>
  constexpr auto CReferenceWrapper<T>::operator ->() const noexcept -> T* {
    return _ptr;
  }

  template <typename T>
  constexpr CReferenceWrapper<T>::operator T&() const noexcept {
    return *_ptr;
  }

  template <typename T>
  template <typename... Args>
  constexpr auto CReferenceWrapper<T>::operator ()(Args&&... args) const noexcept -> decltype(auto) {
    return std::invoke(*_ptr, std::forward<Args>(args)...);
  }

  template <typename T>
  CReferenceWrapper(T&) -> CReferenceWrapper<T>;

  template <typename T>
  CReferenceWrapper(const T&) -> CReferenceWrapper<const T>;

  template <typename T>
  RETINA_INLINE constexpr auto MakeRef(T& ref) noexcept -> CReferenceWrapper<T> {
    return CReferenceWrapper<T>(ref);
  }

  template <typename T>
  RETINA_INLINE constexpr auto MakeRef(CReferenceWrapper<T> ref) noexcept -> CReferenceWrapper<T> {
    return ref;
  }

  template <typename T>
  RETINA_INLINE constexpr auto MakeRef(const T&& ref) noexcept -> CReferenceWrapper<const T> = delete;

  template <typename T>
  RETINA_INLINE constexpr auto MakeConstRef(const T& ref) noexcept -> CReferenceWrapper<const T> {
    return CReferenceWrapper<const T>(ref);
  }

  template <typename T>
  RETINA_INLINE constexpr auto MakeConstRef(CReferenceWrapper<T> ref) noexcept -> CReferenceWrapper<const T> {
    return ref;
  }

  template <typename T>
  RETINA_INLINE constexpr auto MakeConstRef(const T&& ref) noexcept -> CReferenceWrapper<const T> = delete;
}
