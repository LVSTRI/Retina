#pragma once

#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Types.hpp>
#include <Retina/Core/Utility.hpp>

#include <compare>
#include <type_traits>
#include <utility>

namespace Retina::Core {
  template <typename T>
  class CArcPtr {
  public:
    RETINA_INLINE constexpr CArcPtr() noexcept;
    RETINA_INLINE constexpr ~CArcPtr() noexcept;
    RETINA_DECLARE_COPY_MOVE(CArcPtr, RETINA_INLINE constexpr);

    RETINA_INLINE constexpr CArcPtr(T* ptr) noexcept;
    RETINA_INLINE constexpr CArcPtr(std::nullptr_t) noexcept;

    template <typename U>
    constexpr CArcPtr(const CArcPtr<U>& other) noexcept;
    template <typename U>
    constexpr auto operator =(const CArcPtr<U>& other) noexcept -> CArcPtr&;
    template <typename U>
    constexpr CArcPtr(CArcPtr<U>&& other) noexcept;
    template <typename U>
    constexpr auto operator =(CArcPtr<U>&& other) noexcept -> CArcPtr&;

    RETINA_NODISCARD RETINA_INLINE constexpr auto Get() const noexcept -> T*;
    RETINA_INLINE constexpr auto Release() noexcept -> T*;
    RETINA_INLINE constexpr auto Reset() noexcept -> void;

    template <typename U>
    RETINA_NODISCARD RETINA_INLINE constexpr auto As() const noexcept -> CArcPtr<U>;

    template <typename C = std::add_const_t<T>>
      requires (!std::is_const_v<T>)
    RETINA_NODISCARD RETINA_INLINE constexpr auto AsConst() const noexcept -> CArcPtr<C>;

    RETINA_NODISCARD RETINA_INLINE constexpr auto operator !() const noexcept -> bool;
    template <typename U>
    RETINA_NODISCARD RETINA_INLINE constexpr auto operator <=>(const CArcPtr<U>&) const noexcept -> std::strong_ordering;

    RETINA_NODISCARD RETINA_INLINE constexpr auto operator *() const noexcept -> T&;
    RETINA_NODISCARD RETINA_INLINE constexpr auto operator ->() const noexcept -> T*;

    RETINA_NODISCARD RETINA_INLINE constexpr operator bool() const noexcept;

    template <typename C = std::add_const_t<T>>
      requires (!std::is_const_v<T>)
    RETINA_NODISCARD RETINA_INLINE constexpr operator CArcPtr<C>() const noexcept;

    template <typename U>
    friend class CArcPtr;

  private:
    T* _ptr = nullptr;
  };

  template <typename T>
  CArcPtr(T*) -> CArcPtr<T>;

  template <typename T>
  CArcPtr(const T*) -> CArcPtr<const T>;

  template <typename T>
  constexpr CArcPtr<T>::CArcPtr() noexcept = default;

  template <typename T>
  constexpr CArcPtr<T>::~CArcPtr() noexcept {
    Reset();
  }

  template <typename T>
  constexpr CArcPtr<T>::CArcPtr(const CArcPtr& other) noexcept
    : CArcPtr(other._ptr)
  {}

  template <typename T>
  constexpr auto CArcPtr<T>::operator =(const CArcPtr& other) noexcept -> CArcPtr& {
    if (this == &other) {
      return *this;
    }
    return Core::Reconstruct(*this, other);
  }

  template <typename T>
  constexpr CArcPtr<T>::CArcPtr(CArcPtr&& other) noexcept
    : _ptr(std::exchange(other._ptr, nullptr))
  {}

  template <typename T>
  constexpr auto CArcPtr<T>::operator =(CArcPtr&& other) noexcept -> CArcPtr& {
    if (this == &other) {
      return *this;
    }
    return Core::Reconstruct(*this, std::move(other));
  }

  template <typename T>
  constexpr CArcPtr<T>::CArcPtr(T* ptr) noexcept
    : _ptr(ptr)
  {
    if (_ptr) {
      _ptr->Grab();
    }
  }

  template <typename T>
  constexpr CArcPtr<T>::CArcPtr(std::nullptr_t) noexcept
    : _ptr(nullptr)
  {}

  template <typename T>
  template <typename U>
  constexpr CArcPtr<T>::CArcPtr(const CArcPtr<U>& other) noexcept
    : CArcPtr(other._ptr)
  {}

  template <typename T>
  template <typename U>
  constexpr auto CArcPtr<T>::operator =(const CArcPtr<U>& other) noexcept -> CArcPtr& {
    return Core::Reconstruct(*this, other);
  }

  template <typename T>
  template <typename U>
  constexpr CArcPtr<T>::CArcPtr(CArcPtr<U>&& other) noexcept
    : _ptr(std::exchange(other._ptr, nullptr))
  {}

  template <typename T>
  template <typename U>
  constexpr auto CArcPtr<T>::operator =(CArcPtr<U>&& other) noexcept -> CArcPtr& {
    return Core::Reconstruct(*this, std::move(other));
  }

  template <typename T>
  constexpr auto CArcPtr<T>::Get() const noexcept -> T* {
    return _ptr;
  }

  template <typename T>
  constexpr auto CArcPtr<T>::Release() noexcept -> T* {
    if (!_ptr) {
      return nullptr;
    }
    RETINA_ASSERT_WITH(_ptr->Drop() == 0, "Invalid reference count");
    return std::exchange(_ptr, nullptr);
  }

  template <typename T>
  constexpr auto CArcPtr<T>::Reset() noexcept -> void {
    if (_ptr) {
      if (_ptr->Drop() == 0) {
        delete _ptr;
      }
    }
    _ptr = nullptr;
  }

  template <typename T>
  template <typename U>
  constexpr auto CArcPtr<T>::As() const noexcept -> CArcPtr<U> {
    return CArcPtr<U>(reinterpret_cast<U*>(_ptr));
  }

  template <typename T>
  template <typename C>
    requires (!std::is_const_v<T>)
  constexpr auto CArcPtr<T>::AsConst() const noexcept -> CArcPtr<C> {
    return CArcPtr<C>(const_cast<C*>(_ptr));
  }

  template <typename T>
  constexpr auto CArcPtr<T>::operator !() const noexcept -> bool {
    return !_ptr;
  }

  template <typename T>
  template <typename U>
  constexpr auto CArcPtr<T>::operator <=>(const CArcPtr<U>& other) const noexcept -> std::strong_ordering {
    return _ptr <=> other._ptr;
  }

  template <typename T>
  constexpr auto CArcPtr<T>::operator *() const noexcept -> T& {
    return *_ptr;
  }

  template <typename T>
  constexpr auto CArcPtr<T>::operator ->() const noexcept -> T* {
    return _ptr;
  }

  template <typename T>
  constexpr CArcPtr<T>::operator bool() const noexcept {
    return _ptr;
  }

  template <typename T>
  template <typename C>
    requires (!std::is_const_v<T>)
  constexpr CArcPtr<T>::operator CArcPtr<C>() const noexcept {
    return AsConst();
  }
}
