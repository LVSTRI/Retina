#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Enum.hpp>
#include <Retina/Graphics/Forward.hpp>

namespace Retina::Graphics {
  enum class EHeapType {
    E_DEVICE_ONLY = std::to_underlying(
      EMemoryPropertyFlag::E_DEVICE_LOCAL
    ),

    E_DEVICE_MAPPABLE = std::to_underlying(
      EMemoryPropertyFlag::E_DEVICE_LOCAL |
      EMemoryPropertyFlag::E_HOST_VISIBLE |
      EMemoryPropertyFlag::E_HOST_COHERENT
    ),

    E_HOST_ONLY_COHERENT = std::to_underlying(
      EMemoryPropertyFlag::E_HOST_VISIBLE |
      EMemoryPropertyFlag::E_HOST_COHERENT
    ),

    E_HOST_ONLY_CACHED = std::to_underlying(
      EMemoryPropertyFlag::E_HOST_VISIBLE |
      EMemoryPropertyFlag::E_HOST_CACHED
    ),
  };

  struct SBufferMemoryRange {
    uint64 Offset = 0;
    uint64 Size = 0;
  };

  struct SBufferCreateInfo {
    std::string Name;
    EBufferCreateFlag Flags = {};
    EHeapType Heap = {};
    uint64 Capacity = 0;
  };
}
