#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Enums.hpp>
#include <Retina/Graphics/Forward.hpp>

#include <vector>
#include <span>

namespace Retina::Graphics {
  struct SDescriptorLayoutBinding {
    uint32 Count = 0;
    EDescriptorBindingFlag Flags = {};
    EShaderStageFlag Stages = {};
    EDescriptorType Type = {};
  };

  struct SDescriptorLayoutCreateInfo {
    std::string Name;
    EDescriptorLayoutCreateFlag Flags = {};
    std::vector<SDescriptorLayoutBinding> Bindings;
  };
}
