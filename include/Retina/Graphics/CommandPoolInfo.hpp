#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Enums.hpp>

#include <string>

namespace Retina::Graphics {
  struct SCommandPoolCreateInfo {
    std::string Name;
    ECommandPoolCreateFlag Flags;
  };

  const inline auto DEFAULT_COMMAND_POOL_CREATE_INFO = SCommandPoolCreateInfo();
}
