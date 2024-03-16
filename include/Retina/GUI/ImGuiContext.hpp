#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Forward.hpp>

#include <Retina/WSI/Forward.hpp>

namespace Retina::GUI {
  class CImGuiContext {
  public:
    CImGuiContext() noexcept;
    ~CImGuiContext() noexcept;

    RETINA_NODISCARD static auto Make(const Graphics::CDevice& device) noexcept -> Core::CUniquePtr<CImGuiContext>;


  private:
    Core::CArcPtr<Graphics::CDevice> _device;
  };
}
