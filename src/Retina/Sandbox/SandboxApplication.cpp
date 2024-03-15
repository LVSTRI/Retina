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

    RETINA_NODISCARD RETINA_INLINE auto MakeGlobalShadowProjView(const glm::mat4& invProjView, const glm::vec3& lightDirection) noexcept -> glm::mat4 {
      RETINA_PROFILE_SCOPED();
      auto frustumCorners = std::to_array<glm::vec3>({
        { -1.0, -1.0, 0.0 },
        {  1.0, -1.0, 0.0 },
        { -1.0,  1.0, 0.0 },
        {  1.0,  1.0, 0.0 },
        { -1.0, -1.0, 1.0 },
        {  1.0, -1.0, 1.0 },
        { -1.0,  1.0, 1.0 },
        {  1.0,  1.0, 1.0 },
      });
      auto frustumCenter = glm::vec3(0.0f);
      for (auto i = 0_u32; i < 8; ++i) {
        const auto corner = invProjView * glm::vec4(frustumCorners[i], 1.0f);
        frustumCorners[i] = glm::vec3(corner / corner.w);
        frustumCenter += frustumCorners[i];
      }
      frustumCenter /= 8.0f;

      auto minFrustumExtent = glm::vec3(std::numeric_limits<float32>::max());
      auto maxFrustumExtent = glm::vec3(std::numeric_limits<float32>::lowest());
      for (auto i = 0_u32; i < 8; ++i) {
        minFrustumExtent = glm::vec3(glm::min(minFrustumExtent.x, frustumCorners[i].x));
        minFrustumExtent = glm::vec3(glm::min(minFrustumExtent.y, frustumCorners[i].y));
        minFrustumExtent = glm::vec3(glm::min(minFrustumExtent.z, frustumCorners[i].z));
        maxFrustumExtent = glm::vec3(glm::max(maxFrustumExtent.x, frustumCorners[i].x));
        maxFrustumExtent = glm::vec3(glm::max(maxFrustumExtent.y, frustumCorners[i].y));
        maxFrustumExtent = glm::vec3(glm::max(maxFrustumExtent.z, frustumCorners[i].z));
      }

      const auto globalProjection = glm::ortho(
        minFrustumExtent.x,
        maxFrustumExtent.x,
        minFrustumExtent.y,
        maxFrustumExtent.y,
        0.0f,
        1.0f
      );
      const auto globalView = glm::lookAt(frustumCenter + lightDirection * 0.5f, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
      return globalProjection * globalView;
    }

    RETINA_NODISCARD RETINA_INLINE auto MakeShadowCascadeViews(
      const SViewInfo& mainView,
      const glm::vec3& lightDirection
    ) noexcept -> std::vector<SShadowCascadeInfo> {
      RETINA_PROFILE_SCOPED();
      auto result = std::vector<SShadowCascadeInfo>(SHADOW_CASCADE_COUNT);
      constexpr static auto nearPlane = SHADOW_CASCADE_NEAR_PLANE;
      constexpr static auto farPlane = SHADOW_CASCADE_FAR_PLANE;
      const auto uvScaleMatrix = glm::mat4(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
      );

      auto cascadeSplits = std::array<float32, SHADOW_CASCADE_COUNT>();
      {
        constexpr static auto lambda = 0.95f;
        constexpr static auto clipRange = farPlane - nearPlane;
        constexpr static auto minZ = nearPlane;
        constexpr static auto maxZ = nearPlane + clipRange;
        constexpr static auto range = maxZ - minZ;
        constexpr static auto ratio = maxZ / minZ;
        for (auto i = 0_u32; i < SHADOW_CASCADE_COUNT; ++i) {
          const auto power = static_cast<float32>(i + 1) / static_cast<float32>(SHADOW_CASCADE_COUNT);
          const auto splitLog = minZ * glm::pow(glm::abs(ratio), power);
          const auto splitUniform = minZ + range * power;
          const auto distance = lambda * (splitLog - splitUniform) + splitUniform;
          cascadeSplits[i] = (distance - nearPlane) / clipRange;
        }
      }

      const auto mainProjView = mainView.FiniteProjection * mainView.View;
      const auto invMainProjView = glm::inverse(mainProjView);

      for (auto i = 0_u32; i < SHADOW_CASCADE_COUNT; ++i) {
        const auto previousSplit = i == 0 ? nearPlane : cascadeSplits[i - 1];
        const auto currentSplit = cascadeSplits[i];

        auto frustumCenter = glm::vec3(0.0f);
        auto frustumSphereRadius = 0.0f;
        {
          auto frustumCorners = std::to_array<glm::vec3>({
            { -1.0, -1.0, 0.0 },
            {  1.0, -1.0, 0.0 },
            { -1.0,  1.0, 0.0 },
            {  1.0,  1.0, 0.0 },
            { -1.0, -1.0, 1.0 },
            {  1.0, -1.0, 1.0 },
            { -1.0,  1.0, 1.0 },
            {  1.0,  1.0, 1.0 },
          });
          for (auto j = 0_u32; j < 8; ++j) {
            const auto corner = invMainProjView * glm::vec4(frustumCorners[j], 1.0f);
            frustumCorners[j] = glm::vec3(corner / corner.w);
          }
          for (auto j = 0_u32; j < 4; ++j) {
            const auto cornerRay = frustumCorners[j + 4] - frustumCorners[j];
            const auto nearCornerRay = cornerRay * previousSplit;
            const auto farCornerRay = cornerRay * currentSplit;
            frustumCorners[j + 4] = frustumCorners[j] + farCornerRay;
            frustumCorners[j] = frustumCorners[j] + nearCornerRay;
          }
          for (auto j = 0_u32; j < 8; ++j) {
            frustumCenter += frustumCorners[j];
          }
          frustumCenter /= 8.0f;

          for (auto j = 0_u32; j < 8; ++j) {
            frustumSphereRadius = glm::max(frustumSphereRadius, glm::length(frustumCorners[j] - frustumCenter));
          }
          frustumSphereRadius = glm::ceil(frustumSphereRadius * 16.0f) / 16.0f;
        }

        const auto minFrustumExtent = glm::vec3(-frustumSphereRadius);
        const auto maxFrustumExtent = glm::vec3(frustumSphereRadius);
        const auto shadowCascadeExtent = maxFrustumExtent - minFrustumExtent;
        const auto shadowEyePosition = frustumCenter + lightDirection * -minFrustumExtent.z;

        const auto shadowView = glm::lookAt(
          shadowEyePosition,
          frustumCenter,
          glm::vec3(0.0f, 1.0f, 0.0f)
        );

        auto shadowProjection = glm::ortho(
          minFrustumExtent.x,
          maxFrustumExtent.x,
          minFrustumExtent.y,
          maxFrustumExtent.y,
          0.0f,
          shadowCascadeExtent.z
        );
        {
          const auto shadowProjView = shadowProjection * shadowView;
          const auto shadowOrigin = shadowProjView * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
          const auto shadowOriginScaled = shadowOrigin * (SHADOW_CASCADE_RESOLUTION / 2.0f);
          const auto shadowOriginRoundDifference = glm::round(shadowOriginScaled) - shadowOriginScaled;
          const auto shadowOriginOffset = shadowOriginRoundDifference * (2.0f / SHADOW_CASCADE_RESOLUTION);
          shadowProjection[3][0] += shadowOriginOffset.x;
          shadowProjection[3][1] += shadowOriginOffset.y;
        }

        const auto shadowProjView = shadowProjection * shadowView;
        const auto invScaledShadowProjView = glm::inverse(uvScaleMatrix * shadowProjView);
        const auto globalProjView = uvScaleMatrix * MakeGlobalShadowProjView(invMainProjView, lightDirection);

        const auto shadowCascadeCorners = std::to_array<glm::vec3>({
          { globalProjView * glm::vec4(glm::vec3(invScaledShadowProjView * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)), 1.0f) },
          { globalProjView * glm::vec4(glm::vec3(invScaledShadowProjView * glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)), 1.0f) },
        });

        const auto shadowCascadeScale = 1.0f / (shadowCascadeCorners[1] - shadowCascadeCorners[0]);
        const auto shadowCascadeOffset = -shadowCascadeCorners[0];
        const auto shadowCascadeClipDistance = nearPlane + currentSplit * (farPlane - nearPlane);
        result[i] = {
          .Projection = shadowProjection,
          .View = shadowView,
          .ProjView = shadowProjView,
          .Global = globalProjView,
          .Scale = glm::vec4(shadowCascadeScale, 0.0f),
          .Offset = glm::vec4(shadowCascadeOffset, shadowCascadeClipDistance),
        };
      }

      return result;
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

    _model = CMeshletModel::Make(Details::WithAssetPath("Models/Sponza/Sponza.gltf"))
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
    _shadowCascadeBuffer = _device->GetShaderResourceTable().MakeBuffer<SShadowCascadeInfo>(FRAMES_IN_FLIGHT, {
      .Name = "ShadowCascadeBuffer",
      .Heap = Graphics::EHeapType::E_DEVICE_MAPPABLE,
      .Capacity = SHADOW_CASCADE_COUNT,
    });

    if (_model) {
      _meshletBuffer = Details::UploadBufferAsResource(*_device, _model->GetMeshlets(), "MeshletBuffer");
      _meshletInstanceBuffer = Details::UploadBufferAsResource(*_device, _model->GetMeshletInstances(), "MeshletInstanceBuffer");
      _transformBuffer = Details::UploadBufferAsResource(*_device, _model->GetTransforms(), "TransformBuffer");
      _positionBuffer = Details::UploadBufferAsResource(*_device, _model->GetPositions(), "PositionBuffer");
      _vertexBuffer = Details::UploadBufferAsResource(*_device, _model->GetVertices(), "VertexBuffer");
      _indexBuffer = Details::UploadBufferAsResource(*_device, _model->GetIndices(), "IndexBuffer");
      _primitiveBuffer = Details::UploadBufferAsResource(*_device, _model->GetPrimitives(), "PrimitiveBuffer");
    }

    InitializeVisbufferPass();
    InitializeVisbufferResolvePass();
    InitializeTonemapPass();

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
    InitializeVisbufferPass();
    InitializeVisbufferResolvePass();
    InitializeTonemapPass();

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
    auto mainView = SViewInfo();
    {
      constexpr static auto fov = 60.0f;
      const auto aspectRatio = _swapchain->GetWidth() / static_cast<float32>(_swapchain->GetHeight());
      const auto projection = MakeInfiniteReversePerspective(fov, aspectRatio, 0.1f);
      const auto finiteProjection = glm::perspective(glm::radians(fov), aspectRatio, SHADOW_CASCADE_NEAR_PLANE, SHADOW_CASCADE_FAR_PLANE);
      const auto view = _camera->GetViewMatrix();
      const auto projView = projection * view;
      mainView = {
        .Projection = projection,
        .FiniteProjection = finiteProjection,
        .View = view,
        .ProjView = projView,
      };
      viewBuffer->Write(mainView);
    }

    auto& shadowCascadeBuffer = _shadowCascadeBuffer[frameIndex];
    {
      const auto elevation = glm::radians(45.0f);
      const auto azimuth = glm::radians(45.0f);
      const auto direction = glm::vec3(
        glm::cos(elevation) * glm::sin(azimuth),
        glm::sin(elevation),
        glm::cos(elevation) * glm::cos(azimuth)
      );
      const auto shadowCascadeViews = Details::MakeShadowCascadeViews(mainView, glm::normalize(direction));
      shadowCascadeBuffer->Write(shadowCascadeViews);
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
    const auto& shadowCascadeBuffer = _shadowCascadeBuffer[frameIndex];

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
        viewBuffer.GetHandle(),
        shadowCascadeBuffer.GetHandle()
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
      .Barrier({
        .ImageMemoryBarriers = {
          {
            .Image = *_tonemap.MainImage,
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
          },
        },
      })
      .BlitImage(*_tonemap.MainImage, _swapchain->GetCurrentImage(), {})
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
