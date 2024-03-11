#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Enums.hpp>

#include <Retina/WSI/Forward.hpp>

namespace Retina::Graphics {
  struct SSwapchainCreateInfo {
    using FuncWsiMakeSurface = auto (*)(WSI::InstanceHandle, WSI::WindowHandle) noexcept -> WSI::SurfaceHandle;

    std::string Name;
    bool VSync = false;
    FuncWsiMakeSurface MakeSurface = nullptr;
  };

  struct SSwapchainPresentInfo {
    std::vector<std::reference_wrapper<const CBinarySemaphore>> WaitSemaphores;
  };
}
