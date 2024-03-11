#pragma once

#include <Retina/Core/Macros.hpp>

#include <chrono>
#include <cstdint>
#include <string_view>

namespace Retina::Core {
  inline namespace Types {
    using int8 = std::int8_t;
    using int16 = std::int16_t;
    using int32 = std::int32_t;
    using int64 = std::int64_t;
    using uint8 = std::uint8_t;
    using uint16 = std::uint16_t;
    using uint32 = std::uint32_t;
    using uint64 = std::uint64_t;
    using isize = std::ptrdiff_t;
    using usize = std::size_t;
    using uintptr = std::uintptr_t;
    using float32 = float;
    using float64 = double;
    using float128 = long double;
  }

  inline namespace Literals {
    RETINA_NODISCARD constexpr auto operator ""_i8(unsigned long long value) noexcept -> int8 {
      return static_cast<int8>(value);
    }

    RETINA_NODISCARD constexpr auto operator ""_i16(unsigned long long value) noexcept -> int16 {
      return static_cast<int16>(value);
    }

    RETINA_NODISCARD constexpr auto operator ""_i32(unsigned long long value) noexcept -> int32 {
      return static_cast<int32>(value);
    }

    RETINA_NODISCARD constexpr auto operator ""_i64(unsigned long long value) noexcept -> int64 {
      return static_cast<int64>(value);
    }

    RETINA_NODISCARD constexpr auto operator ""_u8(unsigned long long value) noexcept -> uint8 {
      return static_cast<uint8>(value);
    }

    RETINA_NODISCARD constexpr auto operator ""_u16(unsigned long long value) noexcept -> uint16 {
      return static_cast<uint16>(value);
    }

    RETINA_NODISCARD constexpr auto operator ""_u32(unsigned long long value) noexcept -> uint32 {
      return static_cast<uint32>(value);
    }

    RETINA_NODISCARD constexpr auto operator ""_u64(unsigned long long value) noexcept -> uint64 {
      return static_cast<uint64>(value);
    }

    RETINA_NODISCARD constexpr auto operator ""_isize(unsigned long long value) noexcept -> isize {
      return static_cast<isize>(value);
    }

    RETINA_NODISCARD constexpr auto operator ""_usize(unsigned long long value) noexcept -> usize {
      return static_cast<usize>(value);
    }

    RETINA_NODISCARD constexpr auto operator ""_uintptr(unsigned long long value) noexcept -> uintptr {
      return static_cast<uintptr>(value);
    }

    RETINA_NODISCARD constexpr auto operator ""_f32(long double value) noexcept -> float32 {
      return static_cast<float32>(value);
    }

    RETINA_NODISCARD constexpr auto operator ""_f64(long double value) noexcept -> float64 {
      return static_cast<float64>(value);
    }

    RETINA_NODISCARD constexpr auto operator ""_f128(long double value) noexcept -> float128 {
      return static_cast<float128>(value);
    }

    RETINA_NODISCARD constexpr auto operator ""_KiB(unsigned long long value) noexcept -> usize {
      return static_cast<usize>(value * 1024);
    }

    RETINA_NODISCARD constexpr auto operator ""_MiB(unsigned long long value) noexcept -> usize {
      return static_cast<usize>(value * 1024 * 1024);
    }

    RETINA_NODISCARD constexpr auto operator ""_GiB(unsigned long long value) noexcept -> usize {
      return static_cast<usize>(value * 1024 * 1024 * 1024);
    }
  }
}
