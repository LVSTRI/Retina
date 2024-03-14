#include <Retina/Sandbox/SandboxApplication.hpp>

namespace Retina {
  auto MakeApplication() noexcept -> std::unique_ptr<Entry::IApplication> {
    RETINA_PROFILE_SCOPED();
    return std::make_unique<Sandbox::CSandboxApplication>();
  }
}

namespace Retina::Sandbox {
  namespace Details {
    RETINA_NODISCARD RETINA_INLINE auto WithShaderPath(const std::filesystem::path& path) noexcept -> std::filesystem::path {
      RETINA_PROFILE_SCOPED();
      return std::filesystem::path(RETINA_SHADER_DIRECTORY) / path;
    }

    RETINA_NODISCARD RETINA_INLINE auto WithAssetPath(const std::filesystem::path& path) noexcept -> std::filesystem::path {
      RETINA_PROFILE_SCOPED();
      return std::filesystem::path(RETINA_ASSET_DIRECTORY) / path;
    }
  }

  CSandboxApplication::CSandboxApplication() noexcept {
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

    _camera = CCamera::Make(_window->GetInput());

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

    _viewBuffer = _device->GetShaderResourceTable().MakeBuffer<SViewInfo>(FRAMES_IN_FLIGHT, {
      .Name = "ViewBuffer",
      .Heap = Graphics::EHeapType::E_DEVICE_MAPPABLE,
      .Capacity = 1,
    });

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

    _mainImageDepth = Graphics::CImage::Make(*_device, {
      .Name = "MainImageDepth",
      .Width = _swapchain->GetWidth(),
      .Height = _swapchain->GetHeight(),
      .Format = Graphics::EResourceFormat::E_D32_SFLOAT,
      .Usage =
        Graphics::EImageUsageFlag::E_DEPTH_STENCIL_ATTACHMENT,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });

    _mainPipeline = Graphics::CMeshShadingPipeline::Make(*_device, {
      .Name = "MainPipeline",
      .MeshShader = Details::WithShaderPath("Main.mesh.glsl"),
      .FragmentShader = Details::WithShaderPath("Main.frag.glsl"),
      .DescriptorLayouts = {
        _device->GetShaderResourceTable().GetDescriptorLayout(),
      },
      .DepthStencilState = {
        .DepthTestEnable = true,
        .DepthWriteEnable = true,
        .DepthCompareOperator = Graphics::ECompareOperator::E_GREATER,
      },
      .DynamicState = { {
        Graphics::EDynamicState::E_VIEWPORT,
        Graphics::EDynamicState::E_SCISSOR,
      } },
      .RenderingInfo = {
        {
          .ColorAttachmentFormats = { _mainImage->GetFormat() },
          .DepthAttachmentFormat = _mainImageDepth->GetFormat(),
        }
      },
    });

    _model = CModel::Make(Details::WithAssetPath("Models/Bistro/bistro.gltf"))
      .or_else([](const auto& error) -> std::expected<CModel, CModel::EError> {
        RETINA_CORE_ERROR("Failed to load model");
        return std::unexpected(error);
      })
      .transform([](auto&& model) -> std::optional<CModel> {
        RETINA_CORE_INFO("Loaded model");
        return model;
      })
      .value_or(std::nullopt);

    _window->GetEventDispatcher().Attach(this, &CSandboxApplication::OnWindowResize);
    _window->GetEventDispatcher().Attach(this, &CSandboxApplication::OnWindowClose);
    _window->GetEventDispatcher().Attach(this, &CSandboxApplication::OnWindowKeyboard);
    _window->GetEventDispatcher().Attach(this, &CSandboxApplication::OnWindowMouseButton);
    _window->GetEventDispatcher().Attach(this, &CSandboxApplication::OnWindowMousePosition);
    _window->GetEventDispatcher().Attach(this, &CSandboxApplication::OnWindowMouseScroll);
  }

  CSandboxApplication::~CSandboxApplication() noexcept {
    RETINA_PROFILE_SCOPED();
    _device->WaitIdle();
  }

  auto CSandboxApplication::Run() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    while (_isRunning) {
      {
        RETINA_SCOPED_TIMER(_timer);
        OnUpdate();
        OnRender();
      }
      RETINA_MARK_FRAME();
    }
  }
  
  auto CSandboxApplication::OnWindowResize(const WSI::SWindowResizeEvent& windowResizeEvent) noexcept -> bool {
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
      .Format = Graphics::EResourceFormat::E_R8G8B8A8_UNORM,
      .Usage =
        Graphics::EImageUsageFlag::E_COLOR_ATTACHMENT |
        Graphics::EImageUsageFlag::E_TRANSFER_SRC,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });
    _mainImageDepth = Graphics::CImage::Make(*_device, {
      .Name = "MainImageDepth",
      .Width = _swapchain->GetWidth(),
      .Height = _swapchain->GetHeight(),
      .Format = Graphics::EResourceFormat::E_D32_SFLOAT,
      .Usage = Graphics::EImageUsageFlag::E_DEPTH_STENCIL_ATTACHMENT,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });

    OnUpdate();
    OnRender();
    return false;
  }

  auto CSandboxApplication::OnWindowClose(const WSI::SWindowCloseEvent&) noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    _isRunning = false;
    return true;
  }

  auto CSandboxApplication::OnWindowKeyboard(const WSI::SWindowKeyboardEvent&) noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return true;
  }

  auto CSandboxApplication::OnWindowMouseButton(const WSI::SWindowMouseButtonEvent& event) noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    if (event.Action == WSI::EInputAction::E_PRESS && event.Button == WSI::EInputMouse::E_BUTTON_RIGHT) {
      _window->GetInput().SetCursorMode(WSI::EInputCursorMode::E_DISABLED);
    }
    if (event.Action == WSI::EInputAction::E_RELEASE && event.Button == WSI::EInputMouse::E_BUTTON_RIGHT) {
      _window->GetInput().SetCursorMode(WSI::EInputCursorMode::E_NORMAL);
    }
    return true;
  }

  auto CSandboxApplication::OnWindowMousePosition(const WSI::SWindowMousePositionEvent&) noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return true;
  }

  auto CSandboxApplication::OnWindowMouseScroll(const WSI::SWindowMouseScrollEvent&) noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return true;
  }

  auto CSandboxApplication::OnUpdate() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    while (!_swapchain->IsValid()) {
      WSI::WaitEvents();
    }
    WSI::PollEvents();

    _camera->Update(_timer.GetDeltaTime());

    const auto frameIndex = WaitForNextFrameIndex();

    auto& viewBuffer = _viewBuffer[frameIndex];
    {
      const auto aspectRatio = _swapchain->GetWidth() / static_cast<float32>(_swapchain->GetHeight());
      const auto projection = MakeInfiniteReversePerspective(60.0f, aspectRatio, 0.1f);
      const auto view = _camera->GetViewMatrix();
      const auto projView = projection * view;
      viewBuffer->Write(SViewInfo {
        .Projection = projection,
        .View = view,
        .ProjView = projView,
      });
    }
  }

  auto CSandboxApplication::OnRender() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    const auto frameIndex = GetCurrentFrameIndex();
    if (!_swapchain->AcquireNextImage(*_imageAvailableSemaphores[frameIndex])) {
      return;
    }
    _device->Tick();

    const auto& viewBuffer = _viewBuffer[frameIndex];

    auto& commandBuffer = *_commandBuffers[frameIndex];
    commandBuffer.GetCommandPool().Reset();
    commandBuffer
      .Begin()
      .Barrier({
        .ImageMemoryBarriers = {
          {
            .Image = *_mainImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_NONE,
            .DestStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
            .SourceAccess = Graphics::EResourceAccessFlag::E_NONE,
            .DestAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
            .OldLayout = Graphics::EImageLayout::E_UNDEFINED,
            .NewLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
          },
          {
            .Image = *_mainImageDepth,
            .SourceStage = Graphics::EPipelineStageFlag::E_NONE,
            .DestStage = Graphics::EPipelineStageFlag::E_EARLY_FRAGMENT_TESTS,
            .SourceAccess = Graphics::EResourceAccessFlag::E_NONE,
            .DestAccess = Graphics::EResourceAccessFlag::E_DEPTH_STENCIL_ATTACHMENT_WRITE,
            .OldLayout = Graphics::EImageLayout::E_UNDEFINED,
            .NewLayout = Graphics::EImageLayout::E_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
          }
        }
      })
      .BeginRendering({
        .Name = "MainPass",
        .ColorAttachments = {
          {
            .ImageView = _mainImage->GetView(),
            .LoadOperator = Graphics::EAttachmentLoadOperator::E_CLEAR,
            .StoreOperator = Graphics::EAttachmentStoreOperator::E_STORE,
            .ClearValue = Graphics::MakeColorClearValue({ 0.025f, 0.025f, 0.025f, 1.0f }),
          }
        },
        .DepthAttachment = { {
          .ImageView = _mainImageDepth->GetView(),
          .LoadOperator = Graphics::EAttachmentLoadOperator::E_CLEAR,
          .StoreOperator = Graphics::EAttachmentStoreOperator::E_STORE,
          .ClearValue = Graphics::MakeDepthStencilClearValue(0.0f, 0),
        } },
      })
      .SetViewport()
      .SetScissor()
      .BindPipeline(*_mainPipeline)
      .BindShaderResourceTable(_device->GetShaderResourceTable())
      .PushConstants(viewBuffer.GetHandle())
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

  auto CSandboxApplication::WaitForNextFrameIndex() noexcept -> uint32 {
    RETINA_PROFILE_SCOPED();
    return _frameTimeline->WaitForNextHostTimelineValue() % FRAMES_IN_FLIGHT;
  }

  auto CSandboxApplication::GetCurrentFrameIndex() noexcept -> uint32 {
    RETINA_PROFILE_SCOPED();
    return _frameTimeline->GetHostTimelineValue() % FRAMES_IN_FLIGHT;
  }
}
