#pragma once

#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Types.hpp>
#include <Retina/Core/Utility.hpp>

#include <ankerl/unordered_dense.h>

#define RETINA_MAKE_TRANSPARENT_EQUAL_TO_SPECIALIZATION(T)                                              \
  template <>                                                                                           \
  struct ::std::equal_to<T> {                                                                           \
    using is_transparent = void;                                                                        \
    RETINA_NODISCARD constexpr auto operator ()(const T& left, const T& right) const noexcept -> bool { \
      return left == right;                                                                             \
    }                                                                                                   \
  }                                                                                                     \

#define RETINA_MAKE_AVALANCHING_TRANSPARENT_HASH_SPECIALIZATION(T, f)                         \
  template <>                                                                                 \
  struct ::ankerl::unordered_dense::hash<T> {                                                 \
    using is_avalanching = void;                                                              \
    using is_transparent = void;                                                              \
    RETINA_NODISCARD constexpr auto operator ()(const T& x) const noexcept -> ::std::size_t { \
      return (f)(x);                                                                          \
    }                                                                                         \
  }

namespace Retina::Core {
  using namespace ankerl;

  template <typename... Args>
  constexpr auto Hash(Args&&... args) noexcept -> usize {
    using unordered_dense::detail::wyhash::hash;
    const auto data = MakeByteArray(std::forward<Args>(args)...);
    return hash(data.data(), data.size());
  }
}
