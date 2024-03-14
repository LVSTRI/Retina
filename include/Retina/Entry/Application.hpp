#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Entry/Camera.hpp>
#include <Retina/Entry/FrameCounter.hpp>
#include <Retina/Entry/FrameTimer.hpp>

#include <Retina/Graphics/Graphics.hpp>

#include <Retina/WSI/WSI.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <optional>

#define FRAMES_IN_FLIGHT 2

namespace Retina::Entry {
  struct SViewInfo {
    glm::mat4 Projection;
    glm::mat4 View;
    glm::mat4 ProjView;
  };

  class CApplication {
  public:
    CApplication() noexcept;
    ~CApplication() noexcept;
    RETINA_DELETE_COPY(CApplication);
    RETINA_DEFAULT_MOVE(CApplication);

    RETINA_NODISCARD static auto Make() noexcept -> CApplication;

    auto Run() noexcept -> void;

  private:
    auto OnWindowResize(const WSI::SWindowResizeEvent& event) noexcept -> bool;
    auto OnWindowClose(const WSI::SWindowCloseEvent& event) noexcept -> bool;
    auto OnWindowKeyboard(const WSI::SWindowKeyboardEvent& event) noexcept -> bool;
    auto OnWindowMouseButton(const WSI::SWindowMouseButtonEvent& event) noexcept -> bool;
    auto OnWindowMousePosition(const WSI::SWindowMousePositionEvent& event) noexcept -> bool;
    auto OnWindowMouseScroll(const WSI::SWindowMouseScrollEvent& event) noexcept -> bool;

    auto OnUpdate() noexcept -> void;
    auto OnRender() noexcept -> void;

    auto WaitForNextFrameIndex() noexcept -> uint32;
    auto GetCurrentFrameIndex() noexcept -> uint32;

  private:
    bool _isRunning = true;

    CFrameTimer _timer = {};

    std::unique_ptr<CCamera> _camera;

    std::unique_ptr<WSI::CWindow> _window;

    // TODO: Make an actual renderer
    Core::CArcPtr<Graphics::CInstance> _instance;
    Core::CArcPtr<Graphics::CDevice> _device;
    Core::CArcPtr<Graphics::CSwapchain> _swapchain;
    std::vector<Core::CArcPtr<Graphics::CCommandBuffer>> _commandBuffers;

    std::vector<Core::CArcPtr<Graphics::CBinarySemaphore>> _imageAvailableSemaphores;
    std::vector<Core::CArcPtr<Graphics::CBinarySemaphore>> _presentReadySemaphores;
    std::unique_ptr<Graphics::CHostDeviceTimeline> _frameTimeline;

    std::vector<Graphics::CShaderResource<Graphics::CTypedBuffer<SViewInfo>>> _viewBuffer;

    Core::CArcPtr<Graphics::CImage> _mainImage;
    Core::CArcPtr<Graphics::CImage> _mainImageDepth;
    Core::CArcPtr<Graphics::CMeshShadingPipeline> _mainPipeline;
  };
}
