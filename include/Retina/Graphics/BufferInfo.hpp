#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Enums.hpp>
#include <Retina/Graphics/Forward.hpp>

namespace Retina::Graphics {
  struct SBufferCreateInfo {
    std::string Name;
    EBufferCreateFlag Flags = {};
    EMemoryPropertyFlag Memory = {};
    uint64 Capacity = 0;
  };
}
