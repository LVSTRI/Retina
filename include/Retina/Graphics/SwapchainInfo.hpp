#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Enum.hpp>

#include <Retina/WSI/Forward.hpp>

namespace Retina::Graphics {
  struct SSwapchainCreateInfo {
    using FuncWsiMakeSurface = auto (*)(WSI::InstanceHandle, WSI::WindowHandle) noexcept -> WSI::SurfaceHandle;

    std::string Name;
    bool VSync = false;
    FuncWsiMakeSurface MakeSurface = nullptr;
  };

  struct SSwapchainPresentInfo {
    std::vector<Core::CReferenceWrapper<const CBinarySemaphore>> WaitSemaphores;
  };
}
