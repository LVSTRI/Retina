#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Sandbox/Camera.hpp>
#include <Retina/Sandbox/FrameCounter.hpp>
#include <Retina/Sandbox/FrameTimer.hpp>
#include <Retina/Sandbox/Logger.hpp>
#include <Retina/Sandbox/MeshletModel.hpp>
#include <Retina/Sandbox/Model.hpp>

#include <Retina/Entry/Application.hpp>

#include <Retina/Graphics/Graphics.hpp>

#include <Retina/WSI/WSI.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <optional>

#define FRAMES_IN_FLIGHT 2

namespace Retina::Sandbox {
  struct SViewInfo {
    glm::mat4 Projection = {};
    glm::mat4 View = {};
    glm::mat4 ProjView = {};
    glm::vec4 Position = {};
  };

  class CSandboxApplication : public Entry::IApplication {
  public:
    CSandboxApplication() noexcept;
    ~CSandboxApplication() noexcept override;

    auto Run() noexcept -> void override;

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

    auto InitializeGui() noexcept -> void;
    auto InitializeVisbufferPass() noexcept -> void;
    auto InitializeVisbufferResolvePass() noexcept -> void;
    auto InitializeTonemapPass() noexcept -> void;

  private:
    bool _isRunning = true;

    CFrameTimer _timer = {};

    std::optional<CMeshletModel> _model = {};

    Core::CUniquePtr<CCamera> _camera;

    Core::CUniquePtr<WSI::CWindow> _window;

    // TODO: Make an actual renderer
    Core::CArcPtr<Graphics::CInstance> _instance;
    Core::CArcPtr<Graphics::CDevice> _device;
    Core::CArcPtr<Graphics::CSwapchain> _swapchain;
    std::vector<Core::CArcPtr<Graphics::CCommandBuffer>> _commandBuffers;

    std::vector<Core::CArcPtr<Graphics::CBinarySemaphore>> _imageAvailableSemaphores;
    std::vector<Core::CArcPtr<Graphics::CBinarySemaphore>> _presentReadySemaphores;
    Core::CUniquePtr<Graphics::CHostDeviceTimeline> _frameTimeline;

    std::vector<Graphics::CShaderResource<Graphics::CTypedBuffer<SViewInfo>>> _viewBuffer;

    Graphics::CShaderResource<Graphics::CTypedBuffer<SMeshlet>> _meshletBuffer;
    Graphics::CShaderResource<Graphics::CTypedBuffer<SMeshletInstance>> _meshletInstanceBuffer;
    Graphics::CShaderResource<Graphics::CTypedBuffer<glm::mat4>> _transformBuffer;
    Graphics::CShaderResource<Graphics::CTypedBuffer<glm::vec3>> _positionBuffer;
    Graphics::CShaderResource<Graphics::CTypedBuffer<SMeshletVertex>> _vertexBuffer;
    Graphics::CShaderResource<Graphics::CTypedBuffer<uint32>> _indexBuffer;
    Graphics::CShaderResource<Graphics::CTypedBuffer<uint8>> _primitiveBuffer;

    struct {
      bool IsInitialized = false;
      Graphics::CShaderResource<Graphics::CImage> MainImage;
      Graphics::CShaderResource<Graphics::CImage> DepthImage;
      Core::CArcPtr<Graphics::CMeshShadingPipeline> MainPipeline;
    } _visbuffer;

    struct {
      bool IsInitialized = false;
      Graphics::CShaderResource<Graphics::CImage> MainImage;
      Core::CArcPtr<Graphics::CGraphicsPipeline> MainPipeline;
    } _visbufferResolve;

    struct {
      bool IsInitialized = false;
      Graphics::CShaderResource<Graphics::CImage> MainImage;
      Core::CArcPtr<Graphics::CGraphicsPipeline> MainPipeline;
    } _tonemap;
  };
}