#include <Retina/Sandbox/SandboxApplication.hpp>

#include <imgui.h>

#include <ktx.h>

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
    RETINA_NODISCARD RETINA_INLINE constexpr auto PreviousPowerTwo(T value) noexcept -> T {
      return 1 << (sizeof(T) * CHAR_BIT - std::countl_zero(value - 1) - 1);
    }

    template <typename T>
    RETINA_NODISCARD RETINA_INLINE constexpr auto NearestPowerTwo(T value) noexcept -> T {
      return 1 << (sizeof(T) * CHAR_BIT - std::countl_zero(value - 1));
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

    RETINA_NODISCARD RETINA_INLINE auto SampleJitter(uint32 index, uint32 size) noexcept -> glm::vec2 {
      constexpr static auto table = std::to_array<std::pair<float32, float32>>({
        { 0.0000000f, 0.00000000f },
        { 0.5000000f, 0.33333333f },
        { 0.2500000f, 0.66666667f },
        { 0.7500000f, 0.11111111f },
        { 0.1250000f, 0.44444444f },
        { 0.6250000f, 0.77777778f },
        { 0.3750000f, 0.22222222f },
        { 0.8750000f, 0.55555556f },
        { 0.0625000f, 0.88888889f },
        { 0.5625000f, 0.03703704f },
        { 0.3125000f, 0.37037037f },
        { 0.8125000f, 0.70370370f },
        { 0.1875000f, 0.14814815f },
        { 0.6875000f, 0.48148148f },
        { 0.4375000f, 0.81481481f },
        { 0.9375000f, 0.25925926f },
        { 0.0312500f, 0.59259259f },
        { 0.5312500f, 0.92592593f },
        { 0.2812500f, 0.07407407f },
        { 0.7812500f, 0.40740741f },
        { 0.1562500f, 0.74074074f },
        { 0.6562500f, 0.18518519f },
        { 0.4062500f, 0.51851852f },
        { 0.9062500f, 0.85185185f },
        { 0.0937500f, 0.29629630f },
        { 0.5937500f, 0.62962963f },
        { 0.3437500f, 0.96296296f },
        { 0.8437500f, 0.01234568f },
        { 0.2187500f, 0.34567901f },
        { 0.7187500f, 0.67901235f },
        { 0.4687500f, 0.12345679f },
        { 0.9687500f, 0.45679012f },
        { 0.0156250f, 0.79012346f },
        { 0.5156250f, 0.23456790f },
        { 0.2656250f, 0.56790123f },
        { 0.7656250f, 0.90123457f },
        { 0.1406250f, 0.04938272f },
        { 0.6406250f, 0.38271605f },
        { 0.3906250f, 0.71604938f },
        { 0.8906250f, 0.16049383f },
        { 0.0781250f, 0.49382716f },
        { 0.5781250f, 0.82716049f },
        { 0.3281250f, 0.27160494f },
        { 0.8281250f, 0.60493827f },
        { 0.2031250f, 0.93827160f },
        { 0.7031250f, 0.08641975f },
        { 0.4531250f, 0.41975309f },
        { 0.9531250f, 0.75308642f },
        { 0.0468750f, 0.19753086f },
        { 0.5468750f, 0.53086420f },
        { 0.2968750f, 0.86419753f },
        { 0.7968750f, 0.30864198f },
        { 0.1718750f, 0.64197531f },
        { 0.6718750f, 0.97530864f },
        { 0.4218750f, 0.02469136f },
        { 0.9218750f, 0.35802469f },
        { 0.1093750f, 0.69135802f },
        { 0.6093750f, 0.13580247f },
        { 0.3593750f, 0.46913580f },
        { 0.8593750f, 0.80246914f },
        { 0.2343750f, 0.24691358f },
        { 0.7343750f, 0.58024691f },
        { 0.4843750f, 0.91358025f },
        { 0.9843750f, 0.06172840f },
        { 0.0078125f, 0.39506173f },
        { 0.5078125f, 0.72839506f },
        { 0.2578125f, 0.17283951f },
        { 0.7578125f, 0.50617284f },
        { 0.1328125f, 0.83950617f },
        { 0.6328125f, 0.28395062f },
        { 0.3828125f, 0.61728395f },
        { 0.8828125f, 0.95061728f },
        { 0.0703125f, 0.09876543f },
        { 0.5703125f, 0.43209877f },
        { 0.3203125f, 0.76543210f },
        { 0.8203125f, 0.20987654f },
        { 0.1953125f, 0.54320988f },
        { 0.6953125f, 0.87654321f },
        { 0.4453125f, 0.32098765f },
        { 0.9453125f, 0.65432099f },
        { 0.0390625f, 0.98765432f },
        { 0.5390625f, 0.00411523f },
        { 0.2890625f, 0.33744856f },
        { 0.7890625f, 0.67078189f },
        { 0.1640625f, 0.11522634f },
        { 0.6640625f, 0.44855967f },
        { 0.4140625f, 0.78189300f },
        { 0.9140625f, 0.22633745f },
        { 0.1015625f, 0.55967078f },
        { 0.6015625f, 0.89300412f },
        { 0.3515625f, 0.04115226f },
        { 0.8515625f, 0.37448560f },
        { 0.2265625f, 0.70781893f },
        { 0.7265625f, 0.15226337f },
        { 0.4765625f, 0.48559671f },
        { 0.9765625f, 0.81893004f },
        { 0.0234375f, 0.26337449f },
        { 0.5234375f, 0.59670782f },
        { 0.2734375f, 0.93004115f },
        { 0.7734375f, 0.07818930f },
        { 0.1484375f, 0.41152263f },
        { 0.6484375f, 0.74485597f },
        { 0.3984375f, 0.18930041f },
        { 0.8984375f, 0.52263374f },
        { 0.0859375f, 0.85596708f },
        { 0.5859375f, 0.30041152f },
        { 0.3359375f, 0.63374486f },
        { 0.8359375f, 0.96707819f },
        { 0.2109375f, 0.01646091f },
        { 0.7109375f, 0.34979424f },
        { 0.4609375f, 0.68312757f },
        { 0.9609375f, 0.12757202f },
        { 0.0546875f, 0.46090535f },
        { 0.5546875f, 0.79423868f },
        { 0.3046875f, 0.23868313f },
        { 0.8046875f, 0.57201646f },
        { 0.1796875f, 0.90534979f },
        { 0.6796875f, 0.05349794f },
        { 0.4296875f, 0.38683128f },
        { 0.9296875f, 0.72016461f },
        { 0.1171875f, 0.16460905f },
        { 0.6171875f, 0.49794239f },
        { 0.3671875f, 0.83127572f },
        { 0.8671875f, 0.27572016f },
        { 0.2421875f, 0.60905350f },
        { 0.7421875f, 0.94238683f },
        { 0.4921875f, 0.09053498f },
        { 0.9921875f, 0.42386831f },
      });
      const auto [x, y] = table[index % std::min<uint32>(table.size(), size)];
      return glm::vec2(x, y) - 0.5f;
    }

    template <typename T>
    RETINA_NODISCARD RETINA_INLINE auto UploadBufferAsResource(
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

    RETINA_NODISCARD RETINA_INLINE auto UploadTexture(
      const Graphics::CDevice& device,
      std::span<const uint8> bytes,
      bool isNormal,
      std::string_view name = "Texture"
    ) noexcept -> Graphics::CShaderResource<Graphics::CImage> {
      RETINA_PROFILE_SCOPED();
      auto textureHandle = Core::Null<ktxTexture2>();
      ktxTexture2_CreateFromMemory(
        bytes.data(),
        bytes.size(),
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &textureHandle
      );
      if (ktxTexture2_NeedsTranscoding(textureHandle)) {
        if (isNormal) {
          ktxTexture2_TranscodeBasis(textureHandle, KTX_TTF_BC5_RG, KTX_TF_HIGH_QUALITY);
        } else {
          ktxTexture2_TranscodeBasis(textureHandle, KTX_TTF_BC7_RGBA, KTX_TF_HIGH_QUALITY);
        }
      }
      auto staging = Graphics::CTypedBuffer<uint8>::Make(device, {
        .Name = "StagingBuffer",
        .Heap = Graphics::EHeapType::E_HOST_ONLY_CACHED,
        .Capacity = textureHandle->dataSize,
      });
      staging->Write(std::span(textureHandle->pData, textureHandle->dataSize));

      auto image = device.GetShaderResourceTable().MakeImage({
        .Name = name.data(),
        .Width = textureHandle->baseWidth,
        .Height = textureHandle->baseHeight,
        .Levels = textureHandle->numLevels,
        .Usage =
          Graphics::EImageUsageFlag::E_SAMPLED |
          Graphics::EImageUsageFlag::E_TRANSFER_DST,
        .Format = isNormal ?
          Graphics::EResourceFormat::E_BC5_UNORM_BLOCK :
          Graphics::EResourceFormat::E_BC7_SRGB_BLOCK,
        .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
      });
      device.GetGraphicsQueue().Submit([&](Graphics::CCommandBuffer& commands) noexcept {
        commands
          .ImageMemoryBarrier({
            .Image = *image,
            .SourceStage = Graphics::EPipelineStageFlag::E_TOP_OF_PIPE,
            .DestStage = Graphics::EPipelineStageFlag::E_TRANSFER,
            .SourceAccess = Graphics::EResourceAccessFlag::E_NONE,
            .DestAccess = Graphics::EResourceAccessFlag::E_TRANSFER_WRITE,
            .OldLayout = Graphics::EImageLayout::E_UNDEFINED,
            .NewLayout = Graphics::EImageLayout::E_TRANSFER_DST_OPTIMAL,
          });
        for (auto i = 0_u32; i < textureHandle->numLevels; ++i) {
          auto offset = 0_u64;
          ktxTexture_GetImageOffset(ktxTexture(textureHandle), i, 0, 0, &offset);
          commands.CopyBufferToImage(
            *staging,
            *image,
            {
              .Offset = offset,
              .SubresourceRange = {
                .BaseLevel = i,
                .LevelCount = 1,
              },
            }
          );
        }
        commands
          .ImageMemoryBarrier({
            .Image = *image,
            .SourceStage = Graphics::EPipelineStageFlag::E_TRANSFER,
            .DestStage =
              Graphics::EPipelineStageFlag::E_FRAGMENT_SHADER |
              Graphics::EPipelineStageFlag::E_COMPUTE_SHADER,
            .SourceAccess = Graphics::EResourceAccessFlag::E_TRANSFER_WRITE,
            .DestAccess = Graphics::EResourceAccessFlag::E_SHADER_READ,
            .OldLayout = Graphics::EImageLayout::E_TRANSFER_DST_OPTIMAL,
            .NewLayout = Graphics::EImageLayout::E_SHADER_READ_ONLY_OPTIMAL,
          });
      });
      ktxTexture_Destroy(ktxTexture(textureHandle));
      return image;
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

    _pointSampler = _device->GetShaderResourceTable().MakeSampler({
      .Name = "PointSampler",
      .Filter = { Graphics::EFilter::E_NEAREST },
      .Address = { Graphics::ESamplerAddressMode::E_CLAMP_TO_BORDER },
      .MipmapMode = Graphics::ESamplerMipmapMode::E_NEAREST,
      .BorderColor = Graphics::EBorderColor::E_FLOAT_OPAQUE_BLACK,
    });
    _pointSamplerInt = _device->GetShaderResourceTable().MakeSampler({
      .Name = "PointSamplerInt",
      .Filter = { Graphics::EFilter::E_NEAREST },
      .Address = { Graphics::ESamplerAddressMode::E_CLAMP_TO_BORDER },
      .MipmapMode = Graphics::ESamplerMipmapMode::E_NEAREST,
      .BorderColor = Graphics::EBorderColor::E_INT_OPAQUE_BLACK,
    });

    _model = std::move(
      CMeshletModel::Make(Details::WithAssetPath("Models/Bistro/Bistro.gltf"))
        .or_else([](const auto& error) -> std::expected<CMeshletModel, CModel::EError> {
          RETINA_SANDBOX_ERROR("Failed to load model");
          return std::unexpected(error);
        })
        .value()
    );

    _viewBuffer = _device->GetShaderResourceTable().MakeBuffer<SViewInfo>(FRAMES_IN_FLIGHT, {
      .Name = "ViewBuffer",
      .Heap = Graphics::EHeapType::E_DEVICE_MAPPABLE,
      .Capacity = 1,
    });

    UpdateDLSSResolution();
    InitializeGUI();
    InitializeVisbufferPass();
    InitializeVisbufferResolvePass();
    InitializeGBufferPass();
    InitializeTonemapPass();
    InitializeDLSSPass();

    _meshletBuffer = Details::UploadBufferAsResource(*_device, _model.GetMeshlets(), "MeshletBuffer");
    _meshletInstanceBuffer = Details::UploadBufferAsResource(*_device, _model.GetMeshletInstances(), "MeshletInstanceBuffer");
    _transformBuffer = Details::UploadBufferAsResource(*_device, _model.GetTransforms(), "TransformBuffer");
    _positionBuffer = Details::UploadBufferAsResource(*_device, _model.GetPositions(), "PositionBuffer");
    _vertexBuffer = Details::UploadBufferAsResource(*_device, _model.GetVertices(), "VertexBuffer");
    _indexBuffer = Details::UploadBufferAsResource(*_device, _model.GetIndices(), "IndexBuffer");
    _primitiveBuffer = Details::UploadBufferAsResource(*_device, _model.GetPrimitives(), "PrimitiveBuffer");

    {
      const auto modelMaterials = _model.GetMaterials();
      const auto modelTextures = _model.GetTextures();

      auto materials = std::vector<SMaterial>();
      materials.reserve(modelMaterials.size());
      for (const auto& currentMaterial : modelMaterials) {
        auto material = currentMaterial;
        if (currentMaterial.BaseColorTexture != -1) {
          const auto [bytes, _] = modelTextures[currentMaterial.BaseColorTexture];
          const auto resource = Details::UploadTexture(*_device, bytes, false);
          _textures.emplace_back(resource);
          material.BaseColorTexture = resource.GetHandle();
        }

        if (currentMaterial.NormalTexture != -1) {
          const auto [bytes, _] = modelTextures[currentMaterial.NormalTexture];
          const auto resource = Details::UploadTexture(*_device, bytes, true);
          _textures.emplace_back(resource);
          material.NormalTexture = resource.GetHandle();
        }

        materials.emplace_back(material);
      }

      _materialBuffer = Details::UploadBufferAsResource(*_device, std::span<const SMaterial>(materials), "MaterialBuffer");
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

    UpdateDLSSResolution();

    InitializeVisbufferPass();
    InitializeVisbufferResolvePass();
    InitializeGBufferPass();
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

    if (_dlss.ShouldResize) {
      _device->WaitIdle();
      UpdateDLSSResolution();
      InitializeVisbufferPass();
      InitializeVisbufferResolvePass();
      InitializeGBufferPass();
      InitializeTonemapPass();
      InitializeDLSSPass();
    }

    auto& viewBuffer = _viewBuffer[frameIndex];
    auto mainView = SViewInfo();
    {
      _camera->SetMovementSpeed(_cameraState.MovementSpeed);
      _camera->SetViewSensitivity(_cameraState.ViewSensitivity);
      const auto fov = _cameraState.Fov;
      const auto viewportSize = glm::vec2(_visbuffer.MainImage->GetWidth(), _visbuffer.MainImage->GetHeight());
      const auto aspectRatio = viewportSize.x / viewportSize.y;
      const auto projection = MakeInfiniteReversePerspective(fov, aspectRatio, _cameraState.Near);
      const auto jitter = _dlss.AlwaysReset ? glm::vec2(0.0f) : Details::SampleJitter(_frameTimeline->GetHostTimelineValue(), _dlss.JitterSize);
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
            .Image = *_visbufferResolve.AlbedoImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_NONE,
            .DestStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
            .SourceAccess = Graphics::EResourceAccessFlag::E_NONE,
            .DestAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
            .OldLayout = Graphics::EImageLayout::E_UNDEFINED,
            .NewLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
          },
          {
            .Image = *_visbufferResolve.NormalImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_NONE,
            .DestStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
            .SourceAccess = Graphics::EResourceAccessFlag::E_NONE,
            .DestAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
            .OldLayout = Graphics::EImageLayout::E_UNDEFINED,
            .NewLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
          },
          {
            .Image = *_visbufferResolve.ShaderMaterialIdImage,
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
            .ImageView = _visbufferResolve.AlbedoImage->GetView(),
            .LoadOperator = Graphics::EAttachmentLoadOperator::E_DONT_CARE,
            .StoreOperator = Graphics::EAttachmentStoreOperator::E_STORE,
          },
          {
            .ImageView = _visbufferResolve.NormalImage->GetView(),
            .LoadOperator = Graphics::EAttachmentLoadOperator::E_DONT_CARE,
            .StoreOperator = Graphics::EAttachmentStoreOperator::E_STORE,
          },
          {
            .ImageView = _visbufferResolve.ShaderMaterialIdImage->GetView(),
            .LoadOperator = Graphics::EAttachmentLoadOperator::E_DONT_CARE,
            .StoreOperator = Graphics::EAttachmentStoreOperator::E_STORE,
          },
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
        _materialBuffer.GetHandle(),
        _linearSampler.GetHandle(),
        viewBuffer.GetHandle()
      )
      .Draw(3)
      .EndRendering()
      .Barrier({
        .ImageMemoryBarriers = {
          {
            .Image = *_visbufferResolve.AlbedoImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
            .DestStage = Graphics::EPipelineStageFlag::E_FRAGMENT_SHADER,
            .SourceAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
            .DestAccess = Graphics::EResourceAccessFlag::E_SHADER_SAMPLED_READ,
            .OldLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
            .NewLayout = Graphics::EImageLayout::E_SHADER_READ_ONLY_OPTIMAL,
          },
          {
            .Image = *_visbufferResolve.NormalImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
            .DestStage = Graphics::EPipelineStageFlag::E_FRAGMENT_SHADER,
            .SourceAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
            .DestAccess = Graphics::EResourceAccessFlag::E_SHADER_SAMPLED_READ,
            .OldLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
            .NewLayout = Graphics::EImageLayout::E_SHADER_READ_ONLY_OPTIMAL,
          },
          {
            .Image = *_visbufferResolve.ShaderMaterialIdImage,
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
          {
            .Image = *_gbufferPass.MainImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_NONE,
            .DestStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
            .SourceAccess = Graphics::EResourceAccessFlag::E_NONE,
            .DestAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
            .OldLayout = Graphics::EImageLayout::E_UNDEFINED,
            .NewLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
          }
        },
      })
      .BeginRendering({
        .Name = "GBufferResolvePass",
        .ColorAttachments = {
          {
            .ImageView = _gbufferPass.MainImage->GetView(),
            .LoadOperator = Graphics::EAttachmentLoadOperator::E_CLEAR,
            .StoreOperator = Graphics::EAttachmentStoreOperator::E_STORE,
            .ClearValue = Graphics::MakeColorClearValue(0.0f),
          },
        },
      })
      .BindPipeline(*_gbufferPass.MainPipeline)
      .BindShaderResourceTable(_device->GetShaderResourceTable())
      .PushConstants(
        _visbufferResolve.AlbedoImage.GetHandle(),
        _visbufferResolve.NormalImage.GetHandle(),
        _visbufferResolve.ShaderMaterialIdImage.GetHandle(),
        _pointSampler.GetHandle(),
        _pointSamplerInt.GetHandle()
      )
      .Draw(3)
      .EndRendering();
    _dlssInstance->Evaluate(commandBuffer, {
      .Color = *_gbufferPass.MainImage,
      .Depth = *_visbuffer.DepthImage,
      .Velocity = *_visbuffer.VelocityImage,
      .Output = *_dlss.MainImage,
      .JitterOffset = Details::SampleJitter(_frameTimeline->GetHostTimelineValue(), _dlss.JitterSize),
      .MotionVectorScale = _dlss.RenderResolution,
    });
    commandBuffer
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
          ImGui::DragFloat("Near", &_cameraState.Near, 0.01f, 0.0f, 5.0f);
        }

        if (ImGui::CollapsingHeader("DLSS", ImGuiTreeNodeFlags_DefaultOpen)) {
          {
            const auto qualityPresetNames = std::to_array<const char*>({
              "Performance",
              "Balanced",
              "Quality",
              "Native",
            });
            auto currentPresetIndex = [&] noexcept {
              switch (_dlss.Preset) {
                case Graphics::ENvidiaDlssQualityPreset::E_PERFORMANCE: return 0;
                case Graphics::ENvidiaDlssQualityPreset::E_BALANCED: return 1;
                case Graphics::ENvidiaDlssQualityPreset::E_QUALITY: return 2;
                case Graphics::ENvidiaDlssQualityPreset::E_NATIVE: return 3;
              }
            }();
            const auto oldPresetIndex = currentPresetIndex;
            if (ImGui::BeginCombo("Quality Preset", qualityPresetNames[currentPresetIndex])) {
              if (ImGui::Selectable(qualityPresetNames[0])) {
                currentPresetIndex = 0;
              }
              if (ImGui::Selectable(qualityPresetNames[1])) {
                currentPresetIndex = 1;
              }
              if (ImGui::Selectable(qualityPresetNames[2])) {
                currentPresetIndex = 2;
              }
              if (ImGui::Selectable(qualityPresetNames[3])) {
                currentPresetIndex = 3;
              }
              ImGui::EndCombo();

              _dlss.Preset = [&] noexcept {
                switch (currentPresetIndex) {
                  case 0: return Graphics::ENvidiaDlssQualityPreset::E_PERFORMANCE;
                  case 1: return Graphics::ENvidiaDlssQualityPreset::E_BALANCED;
                  case 2: return Graphics::ENvidiaDlssQualityPreset::E_QUALITY;
                  case 3: return Graphics::ENvidiaDlssQualityPreset::E_NATIVE;
                }
              }();
              if (oldPresetIndex != currentPresetIndex) {
                _dlss.ShouldResize = true;
              }
            }
          }
          ImGui::Checkbox("Reset", &_dlss.AlwaysReset);
        }

        if (ImGui::CollapsingHeader("Tonemap", ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::DragFloat("White Point", &_tonemap.WhitePoint, 0.1f, 0.0f, 5.0f);
          ImGui::Checkbox("Passthrough", &_tonemap.IsPassthrough);
        }
      }
      ImGui::End();

      if (ImGui::Begin("Texture Viewer")) {
        if (ImGui::CollapsingHeader("GBuffer", ImGuiTreeNodeFlags_DefaultOpen)) {
          {
            ImGui::SeparatorText("Albedo");
            const auto image = _visbufferResolve.AlbedoImage;
            const auto width = ImGui::GetContentRegionAvail().x;
            const auto height = width / image->GetAspectRatio();
            ImGui::Image(GUI::AsTextureHandle(image), { width, height });
          }
          {
            ImGui::SeparatorText("Normal");
            const auto image = _visbufferResolve.NormalImage;
            const auto width = ImGui::GetContentRegionAvail().x;
            const auto height = width / image->GetAspectRatio();
            ImGui::Image(GUI::AsTextureHandle(image), { width, height });
          }
        }
      }
      ImGui::End();
    });
    commandBuffer
      .Barrier({
        .ImageMemoryBarriers = {
          {
            .Image = *_tonemap.MainImage,
            .SourceStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
            .DestStage = Graphics::EPipelineStageFlag::E_FRAGMENT_SHADER,
            .SourceAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
            .DestAccess = Graphics::EResourceAccessFlag::E_SHADER_SAMPLED_READ,
            .OldLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
            .NewLayout = Graphics::EImageLayout::E_SHADER_READ_ONLY_OPTIMAL,
          },
          {
            .Image = _swapchain->GetCurrentImage(),
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
        .Name = "SwapchainCopy",
        .ColorAttachments = {
          {
            .ImageView = _swapchain->GetCurrentImage().GetView(),
            .LoadOperator = Graphics::EAttachmentLoadOperator::E_DONT_CARE,
            .StoreOperator = Graphics::EAttachmentStoreOperator::E_STORE,
          },
        },
      })
      .SetViewport()
      .SetScissor()
      .BindPipeline(*_tonemap.CopyPipeline)
      .BindShaderResourceTable(_device->GetShaderResourceTable())
      .PushConstants(_tonemap.MainImage.GetHandle(), _pointSampler.GetHandle())
      .Draw(3)
      .EndRendering()
      .ImageMemoryBarrier({
        .Image = _swapchain->GetCurrentImage(),
        .SourceStage = Graphics::EPipelineStageFlag::E_COLOR_ATTACHMENT_OUTPUT,
        .DestStage = Graphics::EPipelineStageFlag::E_NONE,
        .SourceAccess = Graphics::EResourceAccessFlag::E_COLOR_ATTACHMENT_WRITE,
        .DestAccess = Graphics::EResourceAccessFlag::E_NONE,
        .OldLayout = Graphics::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
        .NewLayout = Graphics::EImageLayout::E_PRESENT_SRC_KHR,
      })
      .EndNamedRegion()
      .End();

    _device->GetGraphicsQueue().Submit({
      .CommandBuffers = { commandBuffer },
      .WaitSemaphores = {
        { *_imageAvailableSemaphores[frameIndex], Graphics::EPipelineStageFlag::E_FRAGMENT_SHADER },
      },
      .SignalSemaphores = {
        { *_presentReadySemaphores[frameIndex], Graphics::EPipelineStageFlag::E_BOTTOM_OF_PIPE },
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
      .Width = static_cast<uint32>(_dlss.RenderResolution.x),
      .Height = static_cast<uint32>(_dlss.RenderResolution.y),
      .Format = Graphics::EResourceFormat::E_R32_UINT,
      .Usage =
        Graphics::EImageUsageFlag::E_COLOR_ATTACHMENT |
        Graphics::EImageUsageFlag::E_SAMPLED,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });
    _device->GetShaderResourceTable().Destroy(_visbuffer.VelocityImage);
    _visbuffer.VelocityImage = _device->GetShaderResourceTable().MakeImage({
      .Name = "VisbufferVelocityImage",
      .Width = static_cast<uint32>(_dlss.RenderResolution.x),
      .Height = static_cast<uint32>(_dlss.RenderResolution.y),
      .Format = Graphics::EResourceFormat::E_R16G16_SFLOAT,
      .Usage =
        Graphics::EImageUsageFlag::E_COLOR_ATTACHMENT |
        Graphics::EImageUsageFlag::E_SAMPLED,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });
    _device->GetShaderResourceTable().Destroy(_visbuffer.DepthImage);
    _visbuffer.DepthImage = _device->GetShaderResourceTable().MakeImage({
      .Name = "VisbufferDepthImage",
      .Width = static_cast<uint32>(_dlss.RenderResolution.x),
      .Height = static_cast<uint32>(_dlss.RenderResolution.y),
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
    _device->GetShaderResourceTable().Destroy(_visbufferResolve.AlbedoImage);
    _visbufferResolve.AlbedoImage = _device->GetShaderResourceTable().MakeImage({
      .Name = "VisbufferResolveAlbedoImage",
      .Width = static_cast<uint32>(_dlss.RenderResolution.x),
      .Height = static_cast<uint32>(_dlss.RenderResolution.y),
      .Format = Graphics::EResourceFormat::E_R8G8B8A8_UNORM,
      .Usage =
        Graphics::EImageUsageFlag::E_COLOR_ATTACHMENT |
        Graphics::EImageUsageFlag::E_SAMPLED,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });
    _device->GetShaderResourceTable().Destroy(_visbufferResolve.NormalImage);
    _visbufferResolve.NormalImage = _device->GetShaderResourceTable().MakeImage({
      .Name = "VisbufferResolveNormalImage",
      .Width = static_cast<uint32>(_dlss.RenderResolution.x),
      .Height = static_cast<uint32>(_dlss.RenderResolution.y),
      .Format = Graphics::EResourceFormat::E_R16G16_SFLOAT,
      .Usage =
        Graphics::EImageUsageFlag::E_COLOR_ATTACHMENT |
        Graphics::EImageUsageFlag::E_SAMPLED,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });
    _device->GetShaderResourceTable().Destroy(_visbufferResolve.ShaderMaterialIdImage);
    _visbufferResolve.ShaderMaterialIdImage = _device->GetShaderResourceTable().MakeImage({
      .Name = "VisbufferResolveShaderMaterialIdImage",
      .Width = static_cast<uint32>(_dlss.RenderResolution.x),
      .Height = static_cast<uint32>(_dlss.RenderResolution.y),
      .Format = Graphics::EResourceFormat::E_R32_UINT,
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
            .ColorAttachmentFormats = {
              _visbufferResolve.AlbedoImage->GetFormat(),
              _visbufferResolve.NormalImage->GetFormat(),
              _visbufferResolve.ShaderMaterialIdImage->GetFormat(),
            },
          }
        },
      });
      _visbufferResolve.IsInitialized = true;
    }
  }
  
  auto CSandboxApplication::InitializeGBufferPass() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _device->GetShaderResourceTable().Destroy(_gbufferPass.MainImage);
    _gbufferPass.MainImage = _device->GetShaderResourceTable().MakeImage({
      .Name = "GBufferResolveMainImage",
      .Width = static_cast<uint32>(_dlss.RenderResolution.x),
      .Height = static_cast<uint32>(_dlss.RenderResolution.y),
      .Format = Graphics::EResourceFormat::E_R16G16B16A16_SFLOAT,
      .Usage =
        Graphics::EImageUsageFlag::E_COLOR_ATTACHMENT |
        Graphics::EImageUsageFlag::E_SAMPLED,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });
    if (!_gbufferPass.IsInitialized) {
      _gbufferPass.MainPipeline = Graphics::CGraphicsPipeline::Make(*_device, {
        .Name = "GBufferResolvePipeline",
        .VertexShader = Details::WithShaderPath("Fullscreen.vert.glsl"),
        .FragmentShader = Details::WithShaderPath("GBufferResolve.frag.glsl"),
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
            .ColorAttachmentFormats = { _gbufferPass.MainImage->GetFormat() },
          }
        },
      });
      _gbufferPass.IsInitialized = true;
    }
  }

  auto CSandboxApplication::InitializeTonemapPass() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _device->GetShaderResourceTable().Destroy(_tonemap.MainImage);
    _tonemap.MainImage = _device->GetShaderResourceTable().MakeImage({
      .Name = "TonemapMainImage",
      .Width = static_cast<uint32>(_dlss.OutputResolution.x),
      .Height = static_cast<uint32>(_dlss.OutputResolution.y),
      .Format = Graphics::EResourceFormat::E_R16G16B16A16_SFLOAT,
      .Usage =
        Graphics::EImageUsageFlag::E_COLOR_ATTACHMENT |
        Graphics::EImageUsageFlag::E_SAMPLED,
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
      _tonemap.CopyPipeline = Graphics::CGraphicsPipeline::Make(*_device, {
        .Name = "TonemapCopyPipeline",
        .VertexShader = Details::WithShaderPath("Fullscreen.vert.glsl"),
        .FragmentShader = Details::WithShaderPath("CopySwapchain.frag.glsl"),
        .IncludeDirectories = { RETINA_SHADER_DIRECTORY },
        .DescriptorLayouts = {
          _device->GetShaderResourceTable().GetDescriptorLayout(),
        },
        .ColorBlendState = {
          .Attachments = {
            {
              .BlendEnable = false,
            }
          }
        },
        .DynamicState = { {
          Graphics::EDynamicState::E_VIEWPORT,
          Graphics::EDynamicState::E_SCISSOR,
        } },
        .RenderingInfo = {
          {
            .ColorAttachmentFormats = { _swapchain->GetFormat() },
          }
        },
      });
      _tonemap.IsInitialized = true;
    }
  }

  auto CSandboxApplication::InitializeDLSSPass() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_SANDBOX_WARN("Initializing DLSS Feature");
    _dlss.ShouldResize = false;
    _dlss.ShouldReset = true;
    _dlssInstance->Shutdown();
    _dlssInstance->Initialize({
      .RenderResolution = {
        static_cast<uint32>(_dlss.RenderResolution.x),
        static_cast<uint32>(_dlss.RenderResolution.y),
      },
      .OutputResolution = {
        static_cast<uint32>(_dlss.OutputResolution.x),
        static_cast<uint32>(_dlss.OutputResolution.y),
      },
      .Quality = _dlss.Preset,
      .DepthScale = 1.0f,
      .IsHDR = true,
      .IsReverseDepth = true
    });
    _device->GetShaderResourceTable().Destroy(_dlss.MainImage);
    _dlss.MainImage = _device->GetShaderResourceTable().MakeImage({
      .Name = "DLSSOutputImage",
      .Width = static_cast<uint32>(_dlss.OutputResolution.x),
      .Height = static_cast<uint32>(_dlss.OutputResolution.y),
      .Format = Graphics::EResourceFormat::E_R16G16B16A16_SFLOAT,
      .Usage =
        Graphics::EImageUsageFlag::E_TRANSFER_DST |
        Graphics::EImageUsageFlag::E_STORAGE |
        Graphics::EImageUsageFlag::E_SAMPLED,
      .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });
    _dlss.IsInitialized = true;
  }

  auto CSandboxApplication::UpdateDLSSResolution() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    const auto scalingRatio = Graphics::GetScalingRatioFromQualityPreset(_dlss.Preset);
    _dlss.RenderResolution = {
      static_cast<float32>(_swapchain->GetWidth()) * scalingRatio,
      static_cast<float32>(_swapchain->GetHeight()) * scalingRatio,
    };
    _dlss.OutputResolution = {
      static_cast<float32>(_swapchain->GetWidth()),
      static_cast<float32>(_swapchain->GetHeight()),
    };
    _dlss.JitterSize = [&] -> uint32 {
      const auto ratio = _dlss.OutputResolution.y / _dlss.RenderResolution.y;
      return static_cast<uint32>(glm::round(8.0f * ratio * ratio));
    }();

    const auto lodBias = [&] -> float32 {
      const auto ratio = _dlss.RenderResolution.y / _dlss.OutputResolution.y;
      return glm::log2(ratio) - 1.0f;
    }();

    _device->GetShaderResourceTable().Destroy(_linearSampler);
    _linearSampler = _device->GetShaderResourceTable().MakeSampler({
      .Name = "LinearSampler",
      .Filter = { Graphics::EFilter::E_LINEAR },
      .Address = { Graphics::ESamplerAddressMode::E_REPEAT },
      .MipmapMode = Graphics::ESamplerMipmapMode::E_LINEAR,
      .AnisotropyEnable = true,
      .Anisotropy = 16.0f,
      .LodBias = lodBias,
    });
  }
}
