#pragma once

#include <span>

namespace Retina::Graphics {
  struct SInstanceFeature {
    bool Debug = false;
    bool Surface = false;
  };

  struct SInstanceCreateInfo {
    using FuncGetSurfaceExtensionNames = auto (*)() noexcept -> std::span<const char*>;

    FuncGetSurfaceExtensionNames GetSurfaceExtensionNames = nullptr;
    SInstanceFeature Features = {};
  };
}
