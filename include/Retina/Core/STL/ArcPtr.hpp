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
    CArcPtr() noexcept;
    ~CArcPtr() noexcept;
    RETINA_DECLARE_COPY_MOVE(CArcPtr);

    CArcPtr(T* ptr) noexcept;
    CArcPtr(std::nullptr_t) noexcept;

    RETINA_NODISCARD auto Get() const noexcept -> T*;
    RETINA_NODISCARD auto Release() noexcept -> T*;
    auto Reset() noexcept -> void;

    template <typename U>
    RETINA_NODISCARD auto As() const noexcept -> CArcPtr<U>;

    template <typename C = std::add_const_t<T>>
      requires (!std::is_const_v<T>)
    RETINA_NODISCARD auto AsConst() const noexcept -> CArcPtr<C>;

    RETINA_NODISCARD auto operator !() const noexcept -> bool;
    RETINA_NODISCARD auto operator <=>(const CArcPtr&) const noexcept -> std::strong_ordering;

    RETINA_NODISCARD auto operator *() const noexcept -> T&;
    RETINA_NODISCARD auto operator ->() const noexcept -> T*;

    RETINA_NODISCARD operator bool() const noexcept;

    template <typename C = std::add_const_t<T>>
      requires (!std::is_const_v<T>)
    RETINA_NODISCARD operator CArcPtr<C>() const noexcept;

  private:
    T* _ptr = nullptr;
  };

  template <typename T>
  CArcPtr(T*) -> CArcPtr<T>;

  template <typename T>
  CArcPtr(const T*) -> CArcPtr<const T>;

  template <typename T>
  CArcPtr<T>::CArcPtr() noexcept = default;

  template <typename T>
  CArcPtr<T>::~CArcPtr() noexcept {
    RETINA_PROFILE_SCOPED();
    Reset();
  }

  template <typename T>
  CArcPtr<T>::CArcPtr(const CArcPtr& other) noexcept
    : CArcPtr(other._ptr)
  {
    RETINA_PROFILE_SCOPED();
  }

  template <typename T>
  auto CArcPtr<T>::operator =(const CArcPtr& other) noexcept -> CArcPtr& {
    RETINA_PROFILE_SCOPED();
    if (this == &other) {
      return *this;
    }
    return Core::Reconstruct(*this, other);
  }

  template <typename T>
  CArcPtr<T>::CArcPtr(CArcPtr&& other) noexcept
    : _ptr(std::exchange(other._ptr, nullptr))
  {
    RETINA_PROFILE_SCOPED();
  }

  template <typename T>
  auto CArcPtr<T>::operator =(CArcPtr&& other) noexcept -> CArcPtr& {
    RETINA_PROFILE_SCOPED();
    if (this == &other) {
      return *this;
    }
    return Core::Reconstruct(*this, std::move(other));
  }

  template <typename T>
  CArcPtr<T>::CArcPtr(T* ptr) noexcept
    : _ptr(ptr)
  {
    RETINA_PROFILE_SCOPED();
    if (_ptr) {
      _ptr->Grab();
    }
  }

  template <typename T>
  CArcPtr<T>::CArcPtr(std::nullptr_t) noexcept
    : _ptr(nullptr)
  {
    RETINA_PROFILE_SCOPED();
  }

  template <typename T>
  auto CArcPtr<T>::Get() const noexcept -> T* {
    RETINA_PROFILE_SCOPED();
    return _ptr;
  }

  template <typename T>
  auto CArcPtr<T>::Release() noexcept -> T* {
    RETINA_PROFILE_SCOPED();
    if (!_ptr) {
      return nullptr;
    }
    RETINA_ASSERT_WITH(_ptr->Drop() == 0, "Invalid reference count");
    return std::exchange(_ptr, nullptr);
  }

  template <typename T>
  auto CArcPtr<T>::Reset() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    if (_ptr) {
      if (_ptr->Drop() == 0) {
        delete _ptr;
      }
    }
    _ptr = nullptr;
  }

  template <typename T>
  template <typename U>
  auto CArcPtr<T>::As() const noexcept -> CArcPtr<U> {
    RETINA_PROFILE_SCOPED();
    return CArcPtr<U>(reinterpret_cast<U*>(_ptr));
  }

  template <typename T>
  template <typename C>
    requires (!std::is_const_v<T>)
  auto CArcPtr<T>::AsConst() const noexcept -> CArcPtr<C> {
    RETINA_PROFILE_SCOPED();
    return CArcPtr<C>(const_cast<C*>(_ptr));
  }

  template <typename T>
  auto CArcPtr<T>::operator !() const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return !_ptr;
  }

  template <typename T>
  auto CArcPtr<T>::operator <=>(const CArcPtr&) const noexcept -> std::strong_ordering = default;

  template <typename T>
  auto CArcPtr<T>::operator *() const noexcept -> T& {
    RETINA_PROFILE_SCOPED();
    return *_ptr;
  }

  template <typename T>
  auto CArcPtr<T>::operator ->() const noexcept -> T* {
    RETINA_PROFILE_SCOPED();
    return _ptr;
  }

  template <typename T>
  CArcPtr<T>::operator bool() const noexcept {
    RETINA_PROFILE_SCOPED();
    return _ptr;
  }

  template <typename T>
  template <typename C>
    requires (!std::is_const_v<T>)
  CArcPtr<T>::operator CArcPtr<C>() const noexcept {
    RETINA_PROFILE_SCOPED();
    return AsConst();
  }
}
