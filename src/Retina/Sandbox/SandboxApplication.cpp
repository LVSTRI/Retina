#include <Retina/Sandbox/SandboxApplication.hpp>

#include <imgui.h>

namespace Retina {
  auto MakeApplication() noexcept -> Core::CUniquePtr<Entry::IApplication> {
    RETINA_PROFILE_SCOPED();
    return Core::MakeUnique<Sandbox::CSandboxApplication>();
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
    
    template <typename T>
    RETINA_NODISCARD constexpr auto NextPowerTwo(T value) noexcept -> T {
        auto result = static_cast<T>(1);
        while (result < value) {
            result <<= 1;
        }
        return result;
    }
    
    template <typename T>
    RETINA_NODISCARD constexpr auto DivideRoundUp(T dividend, T divisor) noexcept -> T {
        return (dividend + divisor - 1) / divisor;
    }
    
    template <typename T>
    RETINA_NODISCARD constexpr auto DivideRoundDown(T dividend, T divisor) noexcept -> T {
        return dividend / divisor;
    }
    
    template <typename T>
    RETINA_NODISCARD constexpr auto DivideRoundNearest(T dividend, T divisor) noexcept -> T {
        return (dividend >= 0)
            ? (dividend + divisor / 2) / divisor
            : (dividend - divisor / 2 + 1) / divisor;
    }

    template <typename T>
    RETINA_NODISCARD auto UploadBufferAsResource(
      const Graphics::CDevice& device,
      std::span<const T> data,
      std::string_view name = ""
    ) noexcept -> Graphics::CShaderResource<Graphics::CTypedBuffer<T>> {
      RETINA_PROFILE_SCOPED();
      auto buffer = Graphics::CTypedBuffer<T>::Make(device, {
        .Name = "StagingBuffer",
        .Heap = Graphics::EHeapType::E_HOST_ONLY_COHERENT,
        .Capacity = data.size(),
      });
      buffer->Write(data);
      auto resource = device
        .GetShaderResourceTable()
        .MakeBuffer<T>({
          .Name = name.data(),
          .Heap = Graphics::EHeapType::E_DEVICE_ONLY,
          .Capacity = data.size(),
        });
      device.GetTransferQueue().Submit([&](Graphics::CCommandBuffer& commands) noexcept {
        commands.CopyBuffer(*buffer, *resource, {});
      });
      return resource;
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
        .Focused = true,
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

    _model = CMeshletModel::Make(Details::WithAssetPath("Models/Bistro/bistro.gltf"))
      .or_else([](const auto& error) -> std::expected<CMeshletModel, CModel::EError> {
        RETINA_SANDBOX_ERROR("Failed to load model");
        return std::unexpected(error);
      })
      .transform([](auto&& model) -> std::optional<CMeshletModel> {
        return model;
      })
      .value_or(std::nullopt);

    _viewBuffer = _device->GetShaderResourceTable().MakeBuffer<SViewInfo>(FRAMES_IN_FLIGHT, {
      .Name = "ViewBuffer",
      .Heap = Graphics::EHeapType::E_DEVICE_MAPPABLE,
      .Capacity = 1,
    });

    InitializeGui();
    InitializeVisbufferPass();
    InitializeVisbufferResolvePass();
    InitializeTonemapPass();

    if (_model) {
      _meshletBuffer = Details::UploadBufferAsResource(*_device, _model->GetMeshlets(), "MeshletBuffer");
      _meshletInstanceBuffer = Details::UploadBufferAsResource(*_device, _model->GetMeshletInstances(), "MeshletInstanceBuffer");
      _transformBuffer = Details::UploadBufferAsResource(*_device, _model->GetTransforms(), "TransformBuffer");
      _positionBuffer = Details::UploadBufferAsResource(*_device, _model->GetPositions(), "PositionBuffer");
      _vertexBuffer = Details::UploadBufferAsResource(*_device, _model->GetVertices(), "VertexBuffer");
      _indexBuffer = Details::UploadBufferAsResource(*_device, _model->GetIndices(), "IndexBuffer");
      _primitiveBuffer = Details::UploadBufferAsResource(*_device, _model->GetPrimitives(), "PrimitiveBuffer");
    }

    _window->GetEventDispatcher().Attach(this, &CSandboxApplication::OnWindowResize);
    _window->GetEventDispatcher().Attach(this, &CSandboxApplication::OnWindowClose);
    _window->GetEventDispatcher().Attach(this, &CSandboxApplication::OnWindowMouseButton);
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

  auto CSandboxApplication::OnWindowResize(const WSI::SWindowResizeEvent& windowResizeEvent) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    if (windowResizeEvent.Width == 0 || windowResizeEvent.Height == 0) {
      return;
    }
    _device->WaitIdle();
    _frameTimeline = Graphics::CHostDeviceTimeline::Make(*_device, FRAMES_IN_FLIGHT);
    _swapchain = Graphics::CSwapchain::Recreate(std::move(_swapchain));
    InitializeVisbufferPass();
    InitializeVisbufferResolvePass();
    InitializeTonemapPass();

    OnUpdate();
    OnRender();
  }

  auto CSandboxApplication::OnWindowClose(const WSI::SWindowCloseEvent&) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _isRunning = false;
  }

  auto CSandboxApplication::OnWindowMouseButton(const WSI::SWindowMouseButtonEvent& event) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
      return;
    }
    if (event.Action == WSI::EInputAction::E_PRESS && event.Button == WSI::EInputMouse::E_BUTTON_RIGHT) {
      _window->GetInput().SetCursorMode(WSI::EInputCursorMode::E_DISABLED);
    }
    if (event.Action == WSI::EInputAction::E_RELEASE && event.Button == WSI::EInputMouse::E_BUTTON_RIGHT) {
      _window->GetInput().SetCursorMode(WSI::EInputCursorMode::E_NORMAL);
    }
  }

  auto CSandboxApplication::OnUpdate() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    while (!_swapchain->IsValid()) {
      WSI::WaitEvents();
    }
    WSI::PollEvents();
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
      _camera->Update(_timer.GetDeltaTime());
    }
    const auto frameIndex = WaitForNextFrameIndex();

    auto& viewBuffer = _viewBuffer[frameIndex];
    auto mainView = SViewInfo();
    {
      constexpr static auto fov = 60.0f;
      const auto aspectRatio = _swapchain->GetWidth() / static_cast<float32>(_swapchain->GetHeight());
      const auto projection = MakeInfiniteReversePerspective(fov, aspectRatio, 0.1f);
      const auto view = _camera->GetViewMatrix();
      const auto projView = projection * view;
      mainView = {
        .Projection = projection,
        .View = view,
        .ProjView = projView,
        .Position = glm::vec4(_camera->GetPosition(), 1.0f),
      };
      viewBuffer->Write(mainView);
    }
  }

  auto CSandboxApplication::OnRender() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    const auto frameIndex = GetCurrentFrameIndex();
    if (!_swapchain->AcquireNextImage(*_imageAvailableSemaphores[frameIndex])) {
      return;
    }
    _device->Tick();
    _imGuiContext->NewFrame();

    const auto& viewBuffer = _viewBuffer[frameIndex];

    auto& commandBuffer = *_commandBuffers[frameIndex];
    commandBuffer.GetCommandPool().Reset();
    commandBuffer
      .Begin()
      .Barrier({
        .ImageMemoryBarriers = {
          {
            .Image = *_visbuffer.MainImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_NONE,
            .DestStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
            .SourceAccess = Graphics::EResourceAccessFlag::E_NONE,
            .DestAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
            .OldLayout = Graphics::EImageLayout::E_UNDEFINED,
            .NewLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
          },
          {
            .Image = *_visbuffer.DepthImage,
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
        .Name = "VisbufferMainRaster",
        .ColorAttachments = {
          {
            .ImageView = _visbuffer.MainImage->GetView(),
            .LoadOperator = Graphics::EAttachmentLoadOperator::E_CLEAR,
            .StoreOperator = Graphics::EAttachmentStoreOperator::E_STORE,
            .ClearValue = Graphics::MakeColorClearValue(-1_u32),
          }
        },
        .DepthAttachment = { {
          .ImageView = _visbuffer.DepthImage->GetView(),
          .LoadOperator = Graphics::EAttachmentLoadOperator::E_CLEAR,
          .StoreOperator = Graphics::EAttachmentStoreOperator::E_STORE,
          .ClearValue = Graphics::MakeDepthStencilClearValue(0.0f, 0),
        } },
      })
      .SetViewport()
      .SetScissor()
      .BindPipeline(*_visbuffer.MainPipeline)
      .BindShaderResourceTable(_device->GetShaderResourceTable())
      .PushConstants(
        _meshletBuffer.GetHandle(),
        _meshletInstanceBuffer.GetHandle(),
        _transformBuffer.GetHandle(),
        _positionBuffer.GetHandle(),
        _indexBuffer.GetHandle(),
        _primitiveBuffer.GetHandle(),
        viewBuffer.GetHandle()
      )
      .DrawMeshTasks(_meshletInstanceBuffer->GetSize())
      .EndRendering()
      .Barrier({
        .ImageMemoryBarriers = {
          {
            .Image = *_visbuffer.MainImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
            .DestStage = Graphics::EPipelineStageFlag::E_FRAGMENT_SHADER,
            .SourceAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
            .DestAccess = Graphics::EResourceAccessFlag::E_SHADER_SAMPLED_READ,
            .OldLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
            .NewLayout = Graphics::EImageLayout::E_SHADER_READ_ONLY_OPTIMAL,
          },
          {
            .Image = *_visbuffer.DepthImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_LATE_FRAGMENT_TESTS,
            .DestStage = Graphics::EPipelineStageFlag::E_FRAGMENT_SHADER,
            .SourceAccess = Graphics::EResourceAccessFlag::E_DEPTH_STENCIL_ATTACHMENT_WRITE,
            .DestAccess = Graphics::EResourceAccessFlag::E_SHADER_SAMPLED_READ,
            .OldLayout = Graphics::EImageLayout::E_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .NewLayout = Graphics::EImageLayout::E_SHADER_READ_ONLY_OPTIMAL,
          },
          {
            .Image = *_visbufferResolve.MainImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_NONE,
            .DestStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
            .SourceAccess = Graphics::EResourceAccessFlag::E_NONE,
            .DestAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
            .OldLayout = Graphics::EImageLayout::E_UNDEFINED,
            .NewLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
          },
        },
      })
      .BeginRendering({
        .Name = "VisbufferResolve",
        .ColorAttachments = {
          {
            .ImageView = _visbufferResolve.MainImage->GetView(),
            .LoadOperator = Graphics::EAttachmentLoadOperator::E_DONT_CARE,
            .StoreOperator = Graphics::EAttachmentStoreOperator::E_STORE,
          }
        },
      })
      .SetViewport()
      .SetScissor()
      .BindPipeline(*_visbufferResolve.MainPipeline)
      .BindShaderResourceTable(_device->GetShaderResourceTable())
      .PushConstants(
        _visbuffer.MainImage.GetHandle(),
        _visbuffer.DepthImage.GetHandle(),
        viewBuffer.GetHandle()
      )
      .Draw(3)
      .EndRendering()
      .Barrier({
        .ImageMemoryBarriers = {
          {
            .Image = *_visbufferResolve.MainImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
            .DestStage = Graphics::EPipelineStageFlag::E_FRAGMENT_SHADER,
            .SourceAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
            .DestAccess = Graphics::EResourceAccessFlag::E_SHADER_SAMPLED_READ,
            .OldLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
            .NewLayout = Graphics::EImageLayout::E_SHADER_READ_ONLY_OPTIMAL,
          },
          {
            .Image = *_tonemap.MainImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_NONE,
            .DestStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
            .SourceAccess = Graphics::EResourceAccessFlag::E_NONE,
            .DestAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
            .OldLayout = Graphics::EImageLayout::E_UNDEFINED,
            .NewLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
          },
        },
      })
      .BeginRendering({
        .Name = "Tonemap",
        .ColorAttachments = {
          {
            .ImageView = _tonemap.MainImage->GetView(),
            .LoadOperator = Graphics::EAttachmentLoadOperator::E_DONT_CARE,
            .StoreOperator = Graphics::EAttachmentStoreOperator::E_STORE,
          }
        },
      })
      .SetViewport()
      .SetScissor()
      .BindPipeline(*_tonemap.MainPipeline)
      .BindShaderResourceTable(_device->GetShaderResourceTable())
      .PushConstants(
        _visbufferResolve.MainImage.GetHandle()
      )
      .Draw(3)
      .EndRendering()
      .ImageMemoryBarrier({
        .Image = *_tonemap.MainImage,
        .SourceStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
        .DestStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
        .SourceAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
        .DestAccess =
          Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_READ |
          Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
        .OldLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
        .NewLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
      });
    _imGuiContext->Render(*_tonemap.MainImage, commandBuffer, [&] noexcept {
      ImGui::Begin("Hello, world!");
      ImGui::Image(GUI::AsTextureHandle(_visbuffer.DepthImage.GetHandle()), { 1280, 720 });
      ImGui::End();
    });
    commandBuffer
      .BeginNamedRegion("SwapchainBlit")
      .Barrier({
        .ImageMemoryBarriers = {
          {
            .Image = *_tonemap.MainImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
            .DestStage = Graphics::EPipelineStageFlag::E_BLIT,
            .SourceAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
            .DestAccess = Graphics::EResourceAccessFlag::E_TRANSFER_READ,
            .OldLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
            .NewLayout = Graphics::EImageLayout::E_TRANSFER_SRC_OPTIMAL,
          },
          {
            .Image = _swapchain->GetCurrentImage(),
            .SourceStage = Graphics::EPipelineStageFlag::E_NONE,
            .DestStage = Graphics::EPipelineStageFlag::E_BLIT,
            .SourceAccess = Graphics::EResourceAccessFlag::E_NONE,
            .DestAccess = Graphics::EResourceAccessFlag::E_TRANSFER_WRITE,
            .OldLayout = Graphics::EImageLayout::E_UNDEFINED,
            .NewLayout = Graphics::EImageLayout::E_TRANSFER_DST_OPTIMAL,
          },
        },
      })
      .BlitImage(*_tonemap.MainImage, _swapchain->GetCurrentImage(), {})
      .ImageMemoryBarrier({
        .Image = _swapchain->GetCurrentImage(),
        .SourceStage = Graphics::EPipelineStageFlag::E_BLIT,
        .DestStage = Graphics::EPipelineStageFlag::E_NONE,
        .SourceAccess = Graphics::EResourceAccessFlag::E_TRANSFER_WRITE,
        .DestAccess = Graphics::EResourceAccessFlag::E_NONE,
        .OldLayout = Graphics::EImageLayout::E_TRANSFER_DST_OPTIMAL,
        .NewLayout = Graphics::EImageLayout::E_PRESENT_SRC_KHR,
      })
      .EndNamedRegion()
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

  auto CSandboxApplication::InitializeGui() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _imGuiContext = GUI::CImGuiContext::Make(*_window, *_device, {
      .MaxTimelineDifference = static_cast<uint32>(_frameTimeline->GetMaxTimelineDifference()),
    });
  }

  auto CSandboxApplication::InitializeVisbufferPass() noexcept -> void {
    RETINA_PROFILE_SCOPED();

    _device->GetShaderResourceTable().Destroy(_visbuffer.MainImage);
    _visbuffer.MainImage = _device->GetShaderResourceTable().MakeImage({
      .Name = "VisbufferMainImage",
      .Width = _swapchain->GetWidth(),
      .Height = _swapchain->GetHeight(),
      .Format = Graphics::EResourceFormat::E_R32_UINT,
      .Usage =
        Graphics::EImageUsageFlag::E_COLOR_ATTACHMENT |
        Graphics::EImageUsageFlag::E_SAMPLED,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });

    _device->GetShaderResourceTable().Destroy(_visbuffer.DepthImage);
    _visbuffer.DepthImage = _device->GetShaderResourceTable().MakeImage({
      .Name = "VisbufferDepthImage",
      .Width = _swapchain->GetWidth(),
      .Height = _swapchain->GetHeight(),
      .Format = Graphics::EResourceFormat::E_D32_SFLOAT,
      .Usage =
        Graphics::EImageUsageFlag::E_DEPTH_STENCIL_ATTACHMENT |
        Graphics::EImageUsageFlag::E_SAMPLED,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });
    if (!_visbuffer.IsInitialized) {
      _visbuffer.MainPipeline = Graphics::CMeshShadingPipeline::Make(*_device, {
        .Name = "VisbufferMainPipeline",
        .MeshShader = Details::WithShaderPath("Visbuffer.mesh.glsl"),
        .FragmentShader = Details::WithShaderPath("Visbuffer.frag.glsl"),
        .IncludeDirectories = { RETINA_SHADER_DIRECTORY },
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
            .ColorAttachmentFormats = { _visbuffer.MainImage->GetFormat() },
            .DepthAttachmentFormat = _visbuffer.DepthImage->GetFormat(),
          }
        },
      });
      _visbuffer.IsInitialized = true;
    }
  }

  auto CSandboxApplication::InitializeVisbufferResolvePass() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _device->GetShaderResourceTable().Destroy(_visbufferResolve.MainImage);
    _visbufferResolve.MainImage = _device->GetShaderResourceTable().MakeImage({
      .Name = "VisbufferResolveMainImage",
      .Width = _swapchain->GetWidth(),
      .Height = _swapchain->GetHeight(),
      .Format = Graphics::EResourceFormat::E_R32G32B32A32_SFLOAT,
      .Usage =
        Graphics::EImageUsageFlag::E_COLOR_ATTACHMENT |
        Graphics::EImageUsageFlag::E_SAMPLED,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });
    if (!_visbufferResolve.IsInitialized) {
      _visbufferResolve.MainPipeline = Graphics::CGraphicsPipeline::Make(*_device, {
        .Name = "VisbufferResolveMainPipeline",
        .VertexShader = Details::WithShaderPath("Fullscreen.vert.glsl"),
        .FragmentShader = Details::WithShaderPath("VisbufferResolve.frag.glsl"),
        .IncludeDirectories = { RETINA_SHADER_DIRECTORY },
        .DescriptorLayouts = {
          _device->GetShaderResourceTable().GetDescriptorLayout(),
        },
        .DynamicState = { {
          Graphics::EDynamicState::E_VIEWPORT,
          Graphics::EDynamicState::E_SCISSOR,
        } },
        .RenderingInfo = {
          {
            .ColorAttachmentFormats = { _visbufferResolve.MainImage->GetFormat() },
          }
        },
      });
      _visbufferResolve.IsInitialized = true;
    }
  }

  auto CSandboxApplication::InitializeTonemapPass() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _device->GetShaderResourceTable().Destroy(_tonemap.MainImage);
    _tonemap.MainImage = _device->GetShaderResourceTable().MakeImage({
      .Name = "TonemapMainImage",
      .Width = _swapchain->GetWidth(),
      .Height = _swapchain->GetHeight(),
      .Format = Graphics::EResourceFormat::E_R8G8B8A8_UNORM,
      .Usage =
        Graphics::EImageUsageFlag::E_COLOR_ATTACHMENT |
        Graphics::EImageUsageFlag::E_TRANSFER_SRC,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });
    if (!_tonemap.IsInitialized) {
      _tonemap.MainPipeline = Graphics::CGraphicsPipeline::Make(*_device, {
        .Name = "TonemapPipeline",
        .VertexShader = Details::WithShaderPath("Fullscreen.vert.glsl"),
        .FragmentShader = Details::WithShaderPath("Tonemap.frag.glsl"),
        .IncludeDirectories = { RETINA_SHADER_DIRECTORY },
        .DescriptorLayouts = {
          _device->GetShaderResourceTable().GetDescriptorLayout(),
        },
        .DynamicState = { {
          Graphics::EDynamicState::E_VIEWPORT,
          Graphics::EDynamicState::E_SCISSOR,
        } },
        .RenderingInfo = {
          {
            .ColorAttachmentFormats = { _tonemap.MainImage->GetFormat() },
          }
        },
      });
      _tonemap.IsInitialized = true;
    }
  }
}
