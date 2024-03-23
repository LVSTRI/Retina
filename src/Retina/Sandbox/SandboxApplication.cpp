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
    RETINA_NODISCARD RETINA_INLINE constexpr auto NextPowerTwo(T value) noexcept -> T {
      auto result = static_cast<T>(1);
      while (result < value) {
        result <<= 1;
      }
      return result;
    }
    
    template <typename T>
    RETINA_NODISCARD RETINA_INLINE constexpr auto DivideRoundUp(T dividend, T divisor) noexcept -> T {
      return (dividend + divisor - 1) / divisor;
    }
    
    template <typename T>
    RETINA_NODISCARD RETINA_INLINE constexpr auto DivideRoundDown(T dividend, T divisor) noexcept -> T {
      return dividend / divisor;
    }
    
    template <typename T>
    RETINA_NODISCARD RETINA_INLINE constexpr auto DivideRoundNearest(T dividend, T divisor) noexcept -> T {
      return (dividend >= 0)
        ? (dividend + divisor / 2) / divisor
        : (dividend - divisor / 2 + 1) / divisor;
    }

    RETINA_NODISCARD RETINA_INLINE auto SampleJitter(uint32 index) noexcept -> glm::vec2 {
      const static auto table = std::to_array<glm::vec2>({
        { 0.500000f, 0.333333f },
        { 0.250000f, 0.666667f },
        { 0.750000f, 0.111111f },
        { 0.125000f, 0.444444f },
        { 0.625000f, 0.777778f },
        { 0.375000f, 0.222222f },
        { 0.875000f, 0.555556f },
        { 0.062500f, 0.888889f },
        { 0.562500f, 0.037037f },
        { 0.312500f, 0.370370f },
        { 0.812500f, 0.703704f },
        { 0.187500f, 0.148148f },
        { 0.687500f, 0.481481f },
        { 0.437500f, 0.814815f },
        { 0.937500f, 0.259259f },
        { 0.031250f, 0.592593f },
      });
      return table[index % table.size()] - 0.5f;
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
        .DLSS = true,
      },
    });

    _device = Graphics::CDevice::Make(*_instance, {
      .Name = "MainDevice",
      .Features = {
        .Swapchain = true,
        .MeshShader = true,
        .MemoryBudget = true,
        .MemoryPriority = true,
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

    _dlssInstance = Graphics::CNvidiaDlssFeature::Make(*_device);

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

    InitializeGUI();
    InitializeVisbufferPass();
    InitializeVisbufferResolvePass();
    InitializeTonemapPass();
    InitializeDLSSPass();

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
    InitializeDLSSPass();

    OnUpdate();
    OnRender();
  }

  auto CSandboxApplication::OnWindowClose(const WSI::SWindowCloseEvent&) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _isRunning = false;
  }

  auto CSandboxApplication::OnWindowMouseButton(const WSI::SWindowMouseButtonEvent& event) noexcept -> void {
    RETINA_PROFILE_SCOPED();
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
    const auto frameIndex = WaitForNextFrameIndex();

    auto& viewBuffer = _viewBuffer[frameIndex];
    auto mainView = SViewInfo();
    {
      _camera->SetMovementSpeed(_cameraState.MovementSpeed);
      _camera->SetViewSensitivity(_cameraState.ViewSensitivity);
      const auto fov = _cameraState.Fov;
      const auto viewportSize = glm::vec2(_visbuffer.MainImage->GetWidth(), _visbuffer.MainImage->GetHeight());
      const auto aspectRatio = viewportSize.x / viewportSize.y;
      const auto projection = MakeInfiniteReversePerspective(fov, aspectRatio, _cameraState.Near);
      const auto jitter = _dlss.AlwaysReset ? glm::vec2(0.0f) : Details::SampleJitter(_frameTimeline->GetHostTimelineValue());
      const auto jitterMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f * jitter / viewportSize, 0.0f));
      const auto view = _camera->GetViewMatrix();
      const auto projView = projection * view;

      if (_dlss.ShouldReset || _dlss.AlwaysReset) {
        _dlss.PrevProjection = projection;
        _dlss.PrevView = view;
        _dlss.ShouldReset = false;
      }

      mainView = {
        .Projection = projection,
        .PrevProjection = _dlss.PrevProjection,
        .JitterProj = jitterMatrix * projection,
        .PrevJitterProj = jitterMatrix * _dlss.PrevProjection,
        .View = view,
        .PrevView = _dlss.PrevView,
        .ProjView = projView,
        .PrevProjView = _dlss.PrevProjection * _dlss.PrevView,
        .Position = glm::vec4(_camera->GetPosition(), 1.0f),
      };
      _dlss.PrevProjection = projection;
      _dlss.PrevView = view;

      viewBuffer->Write(mainView);
      _camera->Update(_timer.GetDeltaTime());
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
            .Image = *_visbuffer.VelocityImage,
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
          },
          {
            .ImageView = _visbuffer.VelocityImage->GetView(),
            .LoadOperator = Graphics::EAttachmentLoadOperator::E_CLEAR,
            .StoreOperator = Graphics::EAttachmentStoreOperator::E_STORE,
            .ClearValue = Graphics::MakeColorClearValue(0.0f),
          },
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
            .Image = *_visbuffer.VelocityImage,
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
        _meshletBuffer.GetHandle(),
        _meshletInstanceBuffer.GetHandle(),
        _transformBuffer.GetHandle(),
        _vertexBuffer.GetHandle(),
        _positionBuffer.GetHandle(),
        _indexBuffer.GetHandle(),
        _primitiveBuffer.GetHandle(),
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
          {
            .Image = *_dlss.MainImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_NONE,
            .DestStage =
              Graphics::EPipelineStageFlag::E_TRANSFER |
              Graphics::EPipelineStageFlag::E_COMPUTE_SHADER,
            .SourceAccess = Graphics::EResourceAccessFlag::E_NONE,
            .DestAccess =
              Graphics::EResourceAccessFlag::E_SHADER_READ |
              Graphics::EResourceAccessFlag::E_SHADER_WRITE |
              Graphics::EResourceAccessFlag::E_TRANSFER_WRITE,
            .OldLayout = Graphics::EImageLayout::E_UNDEFINED,
            .NewLayout = Graphics::EImageLayout::E_GENERAL,
          },
        },
      })
      .BeginNamedRegion("DLSSPass");
    _dlssInstance->Evaluate(commandBuffer, {
      .Color = *_visbufferResolve.MainImage,
      .Depth = *_visbuffer.DepthImage,
      .Velocity = *_visbuffer.VelocityImage,
      .Output = *_dlss.MainImage,
      .JitterOffset = Details::SampleJitter(_frameTimeline->GetHostTimelineValue()),
      .MotionVectorScale = {
        _swapchain->GetWidth(),
        _swapchain->GetHeight(),
      },
    });
    commandBuffer
      .EndNamedRegion()
      .ImageMemoryBarrier({
        .Image = *_dlss.MainImage,
        .SourceStage =
          Graphics::EPipelineStageFlag::E_TRANSFER |
          Graphics::EPipelineStageFlag::E_COMPUTE_SHADER,
        .DestStage = Graphics::EPipelineStageFlag::E_FRAGMENT_SHADER,
        .SourceAccess =
          Graphics::EResourceAccessFlag::E_SHADER_READ |
          Graphics::EResourceAccessFlag::E_SHADER_WRITE |
          Graphics::EResourceAccessFlag::E_TRANSFER_WRITE,
        .DestAccess = Graphics::EResourceAccessFlag::E_SHADER_SAMPLED_READ,
        .OldLayout = Graphics::EImageLayout::E_GENERAL,
        .NewLayout = Graphics::EImageLayout::E_SHADER_READ_ONLY_OPTIMAL,
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
        _dlss.MainImage.GetHandle(),
        _tonemap.WhitePoint,
        static_cast<uint32>(_tonemap.IsPassthrough)
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
      if (ImGui::Begin("Info")) {
        ImGui::Text("AFPS: %.2f rad/s", glm::two_pi<float32>() * 1.0f / _timer.GetDeltaTime());
        ImGui::Text("FPS: %.2f", 1.0f / _timer.GetDeltaTime());
        ImGui::Text("Frame Time: %.2f ms", _timer.GetDeltaTime() * 1000.0f);

        ImGui::SeparatorText("Camera Info");
        {
          const auto position = _camera->GetPosition();
          const auto front = _camera->GetFront();
          const auto up = _camera->GetUp();
          const auto right = _camera->GetRight();
          ImGui::Text("Position: (%.2f, %.2f, %.2f)", position.x, position.y, position.z);
          ImGui::Text("Front: (%.2f, %.2f, %.2f)", front.x, front.y, front.z);
          ImGui::Text("Up: (%.2f, %.2f, %.2f)", up.x, up.y, up.z);
          ImGui::Text("Right: (%.2f, %.2f, %.2f)", right.x, right.y, right.z);
          ImGui::Text("Rotation: (%.2f, %.2f)", _camera->GetYaw(), _camera->GetPitch());
        }

        ImGui::SeparatorText("Memory Budget");
        {
          const auto deviceLocalBudget = _device->GetHeapBudget(
            Graphics::EMemoryPropertyFlag::E_DEVICE_LOCAL,
            ~Graphics::EMemoryPropertyFlag::E_DEVICE_LOCAL
          );
          const auto hostVisibleBudget = _device->GetHeapBudget(
            Graphics::EMemoryPropertyFlag::E_HOST_VISIBLE,
            Graphics::EMemoryPropertyFlag::E_DEVICE_LOCAL
          );
          ImGui::Text("Device Local Heap: %.2lf MB / %.2lf MB", deviceLocalBudget.Usage / 1048576.0_f64, deviceLocalBudget.Budget / 1048576.0_f64);
          ImGui::Text("Host Visible Heap: %.2lf MB / %.2lf MB", hostVisibleBudget.Usage / 1048576.0_f64, hostVisibleBudget.Budget / 1048576.0_f64);
        }
      }
      ImGui::End();
      if (ImGui::Begin("Settings")) {
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::DragFloat("FOV", &_cameraState.Fov, 1.0f, 0.0f, 180.0f);
          ImGui::DragFloat("Movement Speed", &_cameraState.MovementSpeed, 0.1f, 0.0f, 100.0f);
          ImGui::DragFloat("View Sensitivity", &_cameraState.ViewSensitivity, 0.01f, 0.0f, 1.0f);
          ImGui::DragFloat("Near", &_cameraState.Near, 0.1f, 0.0f, 5.0f);
        }

        if (ImGui::CollapsingHeader("Tonemap", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::DragFloat("White Point", &_tonemap.WhitePoint, 0.1f, 0.0f, 5.0f);
          ImGui::Checkbox("Passthrough", &_tonemap.IsPassthrough);
        }
      }
      ImGui::End();

      if (ImGui::Begin("Texture Viewer")) {}
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

  auto CSandboxApplication::InitializeGUI() noexcept -> void {
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
    _device->GetShaderResourceTable().Destroy(_visbuffer.VelocityImage);
    _visbuffer.VelocityImage = _device->GetShaderResourceTable().MakeImage({
      .Name = "VisbufferVelocityImage",
      .Width = _swapchain->GetWidth(),
      .Height = _swapchain->GetHeight(),
      .Format = Graphics::EResourceFormat::E_R16G16_SFLOAT,
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
            .ColorAttachmentFormats = {
              _visbuffer.MainImage->GetFormat(),
              _visbuffer.VelocityImage->GetFormat(),
            },
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
      .Format = Graphics::EResourceFormat::E_R16G16B16A16_SFLOAT,
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

  auto CSandboxApplication::InitializeDLSSPass() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_SANDBOX_WARN("Initializing DLSS Feature");
    const auto qualityPreset = Graphics::ENvidiaDlssQualityPreset::E_NATIVE;
    _dlssInstance->Shutdown();
    _dlssInstance->Initialize({
      .RenderResolution = {
        _visbuffer.MainImage->GetWidth(),
        _visbuffer.MainImage->GetHeight(),
      },
      .OutputResolution = {
        _swapchain->GetWidth(),
        _swapchain->GetHeight(),
      },
      .Quality = qualityPreset,
      .DepthScale = 1.0f,
      .IsHDR = true,
      .IsReverseDepth = true
    });
    _device->GetShaderResourceTable().Destroy(_dlss.MainImage);
    _dlss.MainImage = _device->GetShaderResourceTable().MakeImage({
      .Name = "DLSSOutputImage",
      .Width = _swapchain->GetWidth(),
      .Height = _swapchain->GetHeight(),
      .Format = Graphics::EResourceFormat::E_R16G16B16A16_SFLOAT,
      .Usage =
        Graphics::EImageUsageFlag::E_TRANSFER_DST |
        Graphics::EImageUsageFlag::E_STORAGE |
        Graphics::EImageUsageFlag::E_SAMPLED,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });
    _dlss.IsInitialized = true;
  }
}
