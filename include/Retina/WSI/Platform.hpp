#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/WSI/Forward.hpp>
#include <Retina/WSI/WindowInfo.hpp>

#include <span>

namespace Retina::WSI {
  auto Initialize() noexcept -> void;

  auto PollEvents() noexcept -> void;
  auto WaitEvents() noexcept -> void;

  auto GetKeyName(EInputKeyboard key, int32 scancode) noexcept -> const char*;

  RETINA_NODISCARD auto GetSurfaceExtensionNames() noexcept -> std::span<const char*>;
  RETINA_NODISCARD auto MakeSurface(InstanceHandle instance, WindowHandle window) noexcept -> SurfaceHandle;
}
