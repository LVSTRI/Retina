#include <Retina/Entry/Application.hpp>

#include <filesystem>

namespace Retina::Entry {
  namespace Details {
    RETINA_NODISCARD RETINA_INLINE auto GetShaderPath(const std::filesystem::path& path) noexcept -> std::filesystem::path {
      RETINA_PROFILE_SCOPED();
      return std::filesystem::path(RETINA_ENTRY_SHADER_DIRECTORY) / path;

    }
  }

  CApplication::CApplication() noexcept {
    RETINA_PROFILE_SCOPED();
    WSI::Initialize();
    _window = WSI::CWindow::Make({
      .Title = "Retina",
      .Width = 1280,
      .Height = 720,
      .Features = {
        .Resizable = true,
        .Decorated = true,
        .Focused = true
      }
    });

    _instance = Graphics::CInstance::Make({
      .GetSurfaceExtensionNames = WSI::GetSurfaceExtensionNames,
      .Features = {
        .Debug = true,
        .Surface = true,
      },
    });

    _device = Graphics::CDevice::Make(*_instance, {
      .Name = "MainDevice",
      .Features = {
        .Swapchain = true,
        .MeshShader = true,
      },
    });

    _swapchain = Graphics::CSwapchain::Make(*_device, *_window, {
      .Name = "MainSwapchain",
      .VSync = true,
      .MakeSurface = WSI::MakeSurface,
    });

    _commandBuffers = Graphics::CCommandBuffer::Make(_device->GetGraphicsQueue(), FRAMES_IN_FLIGHT, {
      .Name = "MainCommandBuffer",
      .PoolInfo = Graphics::DEFAULT_COMMAND_POOL_CREATE_INFO,
    });

    _imageAvailableSemaphores = Graphics::CBinarySemaphore::Make(*_device, FRAMES_IN_FLIGHT, {
      .Name = "ImageAvailableSemaphore",
    });

    _presentReadySemaphores = Graphics::CBinarySemaphore::Make(*_device, FRAMES_IN_FLIGHT, {
      .Name = "PresentReadySemaphore",
    });

    _frameTimeline = Graphics::CHostDeviceTimeline::Make(*_device, FRAMES_IN_FLIGHT);

    _mainImage = Graphics::CImage::Make(*_device, {
      .Name = "MainImage",
      .Width = _swapchain->GetWidth(),
      .Height = _swapchain->GetHeight(),
      .Format = Graphics::EResourceFormat::E_R8G8B8A8_UNORM,
      .Usage =
        Graphics::EImageUsageFlag::E_COLOR_ATTACHMENT |
        Graphics::EImageUsageFlag::E_TRANSFER_SRC,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });

    _mainPipeline = Graphics::CMeshShadingPipeline::Make(*_device, {
      .Name = "MainPipeline",
      .MeshShader = Details::GetShaderPath("Main.mesh.glsl"),
      .FragmentShader = Details::GetShaderPath("Main.frag.glsl"),
      .DynamicState = { {
        Graphics::EDynamicState::E_VIEWPORT,
        Graphics::EDynamicState::E_SCISSOR,
      } },
      .RenderingInfo = {
        {
          .ColorAttachmentFormats = { _mainImage->GetFormat() },
        }
      },
    });

    _window->GetEventDispatcher().Attach(this, &CApplication::OnWindowResize);
    _window->GetEventDispatcher().Attach(this, &CApplication::OnWindowClose);
    _window->GetEventDispatcher().Attach(this, &CApplication::OnWindowKeyboard);
    _window->GetEventDispatcher().Attach(this, &CApplication::OnWindowMouseButton);
    _window->GetEventDispatcher().Attach(this, &CApplication::OnWindowMousePosition);
    _window->GetEventDispatcher().Attach(this, &CApplication::OnWindowMouseScroll);
  }

  CApplication::~CApplication() noexcept {
    RETINA_PROFILE_SCOPED();
    _device->WaitIdle();
  }

  auto CApplication::Make() noexcept -> CApplication {
    RETINA_PROFILE_SCOPED();
    return {};
  }

  auto CApplication::Run() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    while (_isRunning) {
      OnUpdate();
      OnRender();
      RETINA_MARK_FRAME();
    }
  }

  auto CApplication::OnWindowResize(const WSI::SWindowResizeEvent& windowResizeEvent) noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    if (windowResizeEvent.Width == 0 || windowResizeEvent.Height == 0) {
      return false;
    }
    _device->WaitIdle();
    _frameTimeline = Graphics::CHostDeviceTimeline::Make(*_device, FRAMES_IN_FLIGHT);
    _swapchain = Graphics::CSwapchain::Recreate(std::move(_swapchain));
    _mainImage = Graphics::CImage::Make(*_device, {
      .Name = "MainImage",
      .Width = _swapchain->GetWidth(),
      .Height = _swapchain->GetHeight(),
      .Format = _swapchain->GetFormat(),
      .Usage =
        Graphics::EImageUsageFlag::E_COLOR_ATTACHMENT |
        Graphics::EImageUsageFlag::E_TRANSFER_SRC,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });
    OnRender();
    return false;
  }

  auto CApplication::OnWindowClose(const WSI::SWindowCloseEvent&) noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    _isRunning = false;
    return false;
  }

  auto CApplication::OnWindowKeyboard(const WSI::SWindowKeyboardEvent&) noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return false;
  }

  auto CApplication::OnWindowMouseButton(const WSI::SWindowMouseButtonEvent&) noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return false;
  }

  auto CApplication::OnWindowMousePosition(const WSI::SWindowMousePositionEvent&) noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return false;
  }

  auto CApplication::OnWindowMouseScroll(const WSI::SWindowMouseScrollEvent&) noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return false;
  }

  auto CApplication::OnUpdate() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    while (!_swapchain->IsValid()) {
      WSI::WaitEvents();
    }
    WSI::PollEvents();
  }

  auto CApplication::OnRender() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    const auto frameIndex = WaitForNextFrameIndex();
    if (!_swapchain->AcquireNextImage(*_imageAvailableSemaphores[frameIndex])) {
      return;
    }
    _device->Tick();

    auto& commandBuffer = *_commandBuffers[frameIndex];
    commandBuffer.GetCommandPool().Reset();
    commandBuffer
      .Begin()
      .ImageMemoryBarrier({
        .Image = *_mainImage,
        .SourceStage = Graphics::EPipelineStageFlag::E_NONE,
        .DestStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
        .SourceAccess = Graphics::EResourceAccessFlag::E_NONE,
        .DestAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
        .OldLayout = Graphics::EImageLayout::E_UNDEFINED,
        .NewLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
      })
      .BeginRendering({
        .Name = "MainPass",
        .ColorAttachments = {
          {
            .ImageView = _mainImage->GetView(),
            .LoadOperator = Graphics::EAttachmentLoadOperator::E_CLEAR,
            .StoreOperator = Graphics::EAttachmentStoreOperator::E_STORE,
            .ClearValue = Graphics::MakeClearColorValue({ 0.025f, 0.025f, 0.025f, 1.0f }),
          }
        }
      })
      .SetViewport()
      .SetScissor()
      .BindPipeline(*_mainPipeline)
      .DrawMeshTasks(1)
      .EndRendering()
      .Barrier({
        .ImageMemoryBarriers = {
          {
            .Image = *_mainImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
            .DestStage = Graphics::EPipelineStageFlag::E_TRANSFER,
            .SourceAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
            .DestAccess = Graphics::EResourceAccessFlag::E_TRANSFER_READ,
            .OldLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
            .NewLayout = Graphics::EImageLayout::E_TRANSFER_SRC_OPTIMAL,
          },
          {
            .Image = _swapchain->GetCurrentImage(),
            .SourceStage = Graphics::EPipelineStageFlag::E_NONE,
            .DestStage = Graphics::EPipelineStageFlag::E_TRANSFER,
            .SourceAccess = Graphics::EResourceAccessFlag::E_NONE,
            .DestAccess = Graphics::EResourceAccessFlag::E_TRANSFER_WRITE,
            .OldLayout = Graphics::EImageLayout::E_UNDEFINED,
            .NewLayout = Graphics::EImageLayout::E_TRANSFER_DST_OPTIMAL,
          }
        }
      })
      .BlitImage(*_mainImage, _swapchain->GetCurrentImage(), {})
      .ImageMemoryBarrier({
        .Image = _swapchain->GetCurrentImage(),
        .SourceStage = Graphics::EPipelineStageFlag::E_TRANSFER,
        .DestStage = Graphics::EPipelineStageFlag::E_NONE,
        .SourceAccess = Graphics::EResourceAccessFlag::E_TRANSFER_WRITE,
        .DestAccess = Graphics::EResourceAccessFlag::E_NONE,
        .OldLayout = Graphics::EImageLayout::E_TRANSFER_DST_OPTIMAL,
        .NewLayout = Graphics::EImageLayout::E_PRESENT_SRC_KHR,
      })
      .End();

    _device->GetGraphicsQueue().Submit({
      .CommandBuffers = { commandBuffer },
      .WaitSemaphores = {
        { *_imageAvailableSemaphores[frameIndex], Graphics::EPipelineStageFlag::E_TRANSFER },
      },
      .SignalSemaphores = {
        { *_presentReadySemaphores[frameIndex], Graphics::EPipelineStageFlag::E_TRANSFER },
      },
      .Timelines = {
        *_frameTimeline,
        _device->GetMainTimeline(),
      }
    });

    if (!_swapchain->Present({ { *_presentReadySemaphores[frameIndex] } })) {
      return;
    }
  }

  auto CApplication::WaitForNextFrameIndex() noexcept -> uint32 {
    RETINA_PROFILE_SCOPED();
    return _frameTimeline->WaitForNextHostTimelineValue() % FRAMES_IN_FLIGHT;
  }
}
