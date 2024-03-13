#pragma once

#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Types.hpp>

#include <array>
#include <bit>

namespace Retina::Core {
  template <usize N>
    requires (N % 64 == 0)
  class CSlotAllocator {
  public:
    constexpr CSlotAllocator() noexcept = default;
    constexpr ~CSlotAllocator() noexcept = default;
    RETINA_DEFAULT_COPY_MOVE(CSlotAllocator, constexpr);

    RETINA_NODISCARD RETINA_INLINE constexpr auto Allocate() noexcept -> uint64;
    RETINA_NODISCARD RETINA_INLINE constexpr auto Free(uint64 slot) noexcept -> void;

  private:
    std::array<uint64, N / 64> _slots = {};
  };

  template <usize N>
    requires (N % 64 == 0)
  constexpr auto CSlotAllocator<N>::Allocate() noexcept -> uint64 {
    for (usize i = 0; i < _slots.size(); ++i) {
      if (_slots[i] != -1_u64) {
        const auto bit = std::countr_one(_slots[i]);
        _slots[i] |= 1_u64 << bit;
        return i * 64 + bit;
      }
    }
    return -1_u64;
  }

  template <usize N>
    requires (N % 64 == 0)
  constexpr auto CSlotAllocator<N>::Free(uint64 slot) noexcept -> void {
    const auto index = slot / 64;
    const auto bit = slot % 64;
    _slots[index] &= ~(1_u64 << bit);
  }
}
