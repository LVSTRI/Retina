#pragma once

#include <Retina/Core/Core.hpp>

#include <string>

namespace Retina::Graphics {
  enum class ESemaphoreKind {
    E_BINARY,
    E_TIMELINE,
  };

  struct SBinarySemaphoreCreateInfo {
    std::string Name;
  };

  struct STimelineSemaphoreCreateInfo {
    std::string Name;
    uint64 Value = 0;
  };
}
