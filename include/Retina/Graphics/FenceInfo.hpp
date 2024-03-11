#pragma once

#include <Retina/Core/Core.hpp>

#include <string>

namespace Retina::Graphics {
  struct SFenceCreateInfo {
    std::string Name;
    bool IsSignaled = false;
  };
}
