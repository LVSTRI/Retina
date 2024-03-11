#pragma once

#include <Retina/Core/STL/ArcPtr.hpp>

#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Types.hpp>

#include <atomic>

namespace Retina::Core {
  template <typename T>
  class IEnableIntrusiveReferenceCount {
  public:
    RETINA_DELETE_COPY_MOVE(IEnableIntrusiveReferenceCount);

    RETINA_NODISCARD auto Count() const noexcept -> uint64;
    auto Grab() const noexcept -> uint64;
    auto Drop() const noexcept -> uint64;

    RETINA_NODISCARD auto ToArcPtr() noexcept -> CArcPtr<T>;
    RETINA_NODISCARD auto ToArcPtr() const noexcept -> CArcPtr<const T>;

  protected:
    IEnableIntrusiveReferenceCount() noexcept = default;
    ~IEnableIntrusiveReferenceCount() noexcept = default;

  private:
    mutable std::atomic<uint64> _count = 0;
  };

  template <typename T>
  auto IEnableIntrusiveReferenceCount<T>::Count() const noexcept -> uint64 {
    RETINA_PROFILE_SCOPED();
    return _count;
  }

  template <typename T>
  auto IEnableIntrusiveReferenceCount<T>::Grab() const noexcept -> uint64 {
    RETINA_PROFILE_SCOPED();
    return ++_count;
  }

  template <typename T>
  auto IEnableIntrusiveReferenceCount<T>::Drop() const noexcept -> uint64 {
    RETINA_PROFILE_SCOPED();
    return --_count;
  }

  template <typename T>
  auto IEnableIntrusiveReferenceCount<T>::ToArcPtr() noexcept -> CArcPtr<T> {
    RETINA_PROFILE_SCOPED();
    return CArcPtr(static_cast<T*>(this));
  }

  template <typename T>
  auto IEnableIntrusiveReferenceCount<T>::ToArcPtr() const noexcept -> CArcPtr<const T> {
    RETINA_PROFILE_SCOPED();
    return CArcPtr(static_cast<const T*>(this));
  }
}
