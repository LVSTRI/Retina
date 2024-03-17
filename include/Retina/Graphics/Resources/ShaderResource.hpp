#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Enum.hpp>
#include <Retina/Graphics/Forward.hpp>

namespace Retina::Graphics {
  template <typename T>
  class CShaderResource {
  public:
    CShaderResource() noexcept = default;
    ~CShaderResource() noexcept = default;

    RETINA_NODISCARD RETINA_INLINE static auto Make(T& resource, uint32 handle) noexcept -> CShaderResource;

    RETINA_NODISCARD RETINA_INLINE auto GetResource() const noexcept -> const T&;
    RETINA_NODISCARD RETINA_INLINE auto GetHandle() const noexcept -> uint32;

    RETINA_NODISCARD RETINA_INLINE auto IsValid() const noexcept -> bool;

    RETINA_NODISCARD RETINA_INLINE auto operator *() noexcept -> T&;
    RETINA_NODISCARD RETINA_INLINE auto operator *() const noexcept -> const T&;
    RETINA_NODISCARD RETINA_INLINE auto operator ->() noexcept -> T*;
    RETINA_NODISCARD RETINA_INLINE auto operator ->() const noexcept -> const T*;

    RETINA_NODISCARD RETINA_INLINE operator bool() const noexcept;
    RETINA_NODISCARD RETINA_INLINE auto operator !() const noexcept -> bool;

    RETINA_NODISCARD RETINA_INLINE auto operator <=>(const CShaderResource&) const noexcept -> std::strong_ordering = default;

    RETINA_DECLARE_FRIEND_HASH(CShaderResource);

  private:
    T* _resource = nullptr;
    uint32 _handle = -1_u32;
  };

  template <typename T>
  auto CShaderResource<T>::Make(T& resource, uint32 handle) noexcept -> CShaderResource {
    RETINA_PROFILE_SCOPED();
    auto self = CShaderResource();
    self._resource = &resource;
    self._handle = handle;
    return self;
  }

  template <typename T>
  auto CShaderResource<T>::GetHandle() const noexcept -> uint32 {
    RETINA_PROFILE_SCOPED();
    return _handle;
  }

  template <typename T>
  auto CShaderResource<T>::GetResource() const noexcept -> const T& {
    RETINA_PROFILE_SCOPED();
    return *_resource;
  }

  template <typename T>
  auto CShaderResource<T>::IsValid() const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return _handle != -1_u32;
  }

  template <typename T>
  auto CShaderResource<T>::operator *() noexcept -> T& {
    RETINA_PROFILE_SCOPED();
    RETINA_ASSERT_WITH(IsValid(), "Invalid shader resource");
    return *_resource;
  }

  template <typename T>
  auto CShaderResource<T>::operator *() const noexcept -> const T& {
    RETINA_PROFILE_SCOPED();
    RETINA_ASSERT_WITH(IsValid(), "Invalid shader resource");
    return *_resource;
  }

  template <typename T>
  auto CShaderResource<T>::operator ->() noexcept -> T* {
    RETINA_PROFILE_SCOPED();
    RETINA_ASSERT_WITH(IsValid(), "Invalid shader resource");
    return _resource;
  }

  template <typename T>
  auto CShaderResource<T>::operator ->() const noexcept -> const T* {
    RETINA_PROFILE_SCOPED();
    RETINA_ASSERT_WITH(IsValid(), "Invalid shader resource");
    return _resource;
  }

  template <typename T>
  CShaderResource<T>::operator bool() const noexcept {
    RETINA_PROFILE_SCOPED();
    return IsValid();
  }

  template <typename T>
  auto CShaderResource<T>::operator !() const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return !IsValid();
  }
}

template <typename T>
RETINA_MAKE_TRANSPARENT_EQUAL_TO_SPECIALIZATION(Retina::Graphics::CShaderResource<T>);

template <typename T>
RETINA_MAKE_AVALANCHING_TRANSPARENT_HASH_SPECIALIZATION(
  Retina::Graphics::CShaderResource<T>,
  [](const Retina::Graphics::CShaderResource<T>& resource) {
    return Retina::Core::Hash(resource._resource, resource._handle);
  }
);
