#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Enums.hpp>
#include <Retina/Graphics/Forward.hpp>

namespace Retina::Graphics {
  struct SDescriptorPoolSize {
    EDescriptorType Type = {};
    uint32 Count = 0;
  };

  struct SDescriptorPoolCreateInfo {
    std::string Name;
    EDescriptorPoolCreateFlag Flags = {};
    uint32 MaxSets = 0;
    std::vector<SDescriptorPoolSize> PoolSizes;
  };
}
