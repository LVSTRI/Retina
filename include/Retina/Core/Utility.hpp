#pragma once

#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Types.hpp>

#include <cstdlib>
#include <memory>
#include <array>

namespace Retina::Core {
  namespace Details {
    template <typename T, typename U>
    constexpr auto MakeByteArrayImpl(T&& value, U& output, usize& offset) noexcept -> void {
      __builtin_memcpy(&output[offset], &value, sizeof(T));
      offset += sizeof(T);
    }
  }

  template <typename... Ts>
  RETINA_NODISCARD consteval auto GetSizeBytesFromPack() noexcept -> usize {
    return (sizeof(Ts) + ...);
  }

  template <typename T>
  RETINA_NODISCARD constexpr auto AsConstPtr(const T& value) noexcept -> const T* {
    return &value;
  }

  template <typename T>
  RETINA_NODISCARD constexpr auto AsConstPtr(const T(&value)[]) noexcept -> const T* {
    return &value[0];
  }

  template <typename T>
  RETINA_NODISCARD constexpr auto AsConstPtr(const T&& value) noexcept -> const T* {
    return &value;
  }

  template <typename T>
  RETINA_NODISCARD constexpr auto AsConstPtr(const T(&&value)[]) noexcept -> const T* {
    return &value[0];
  }

  template <typename T>
  RETINA_NODISCARD constexpr auto AsMutPtr(T&& value) noexcept -> T* {
    return &value;
  }

  template <typename T>
  RETINA_NODISCARD constexpr auto AsMutPtr(T(&&value)[]) noexcept -> T* {
    return &value[0];
  }

  template <typename T>
  RETINA_NODISCARD constexpr auto AsConstRef(const T& value) noexcept -> const T& {
    return value;
  }

  template <typename T>
  RETINA_NODISCARD constexpr auto AsConstRef(const T(&value)[]) noexcept -> const T& {
    return value[0];
  }

  template <typename T>
  RETINA_NODISCARD constexpr auto AsConstRef(const T&& value) noexcept -> const T& {
    return value;
  }

  template <typename T>
  RETINA_NODISCARD constexpr auto AsConstRef(const T(&&value)[]) noexcept -> const T& {
    return value[0];
  }

  template <typename T>
  RETINA_NODISCARD constexpr auto AsMutRef(T&& value) noexcept -> T& {
    return *AsMutPtr(std::forward<T>(value));
  }

  template <typename T>
  RETINA_NODISCARD constexpr auto AsMutRef(T(&&value)[]) noexcept -> T& {
    return *AsMutPtr(std::forward<T[]>(value));
  }

  template <typename T, typename... Args>
  RETINA_NODISCARD constexpr auto Construct(T* where, Args&&... args) noexcept -> T* {
    return std::construct_at(where, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  RETINA_NODISCARD constexpr auto Construct(T& where, Args&&... args) noexcept -> T& {
    return *std::construct_at(std::addressof(where), std::forward<Args>(args)...);
  }

  template <typename T>
  constexpr auto Destroy(T* where) noexcept -> void {
    std::destroy_at(where);
  }

  template <typename T>
  constexpr auto Destroy(T& where) noexcept -> void {
    std::destroy_at(std::addressof(where));
  }

  template <typename T, typename... Args>
  RETINA_NODISCARD constexpr auto Reconstruct(T* where, Args&&... args) noexcept -> T* {
    Destroy(where);
    return Construct(where, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  RETINA_NODISCARD constexpr auto Reconstruct(T& where, Args&&... args) noexcept -> T& {
    Destroy(where);
    return Construct(where, std::forward<Args>(args)...);
  }

  template <typename... Args, typename R = std::array<uint8, GetSizeBytesFromPack<Args...>()>>
  RETINA_NODISCARD constexpr auto MakeByteArray(Args&&... args) noexcept -> R {
    auto data = R();
    auto offset = 0_usize;
    (Details::MakeByteArrayImpl(std::forward<Args>(args), data, offset), ...);
    return data;
  }

  template <typename E>
  RETINA_NODISCARD constexpr auto IsFlagEnabled(const E& value, const E& flag) noexcept -> bool {
    return (value & flag) == flag;
  }

  RETINA_NODISCARD constexpr auto MultiByteToWideString(std::string_view input) noexcept -> std::wstring {
    auto output = std::wstring(input.size(), L'\0');
    const auto size = std::mbstowcs(output.data(), input.data(), input.size());
    if (size == -1_usize) {
      return {};
    }
    output.resize(size);
    return output;
  }
}
