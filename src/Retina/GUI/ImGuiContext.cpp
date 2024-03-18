#include <Retina/Graphics/Resources/ShaderResourceTable.hpp>
#include <Retina/Graphics/CommandBuffer.hpp>
#include <Retina/Graphics/DescriptorLayout.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/GraphicsPipeline.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/ImageView.hpp>
#include <Retina/Graphics/Sampler.hpp>
#include <Retina/Graphics/Queue.hpp>

#include <Retina/GUI/ImGuiContext.hpp>
#include <Retina/GUI/Logger.hpp>

#include <Retina/WSI/Input.hpp>
#include <Retina/WSI/Platform.hpp>
#include <Retina/WSI/Window.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>

#include <filesystem>

namespace Retina::GUI {
  namespace Details {
    RETINA_NODISCARD RETINA_INLINE auto WithShaderPath(const std::filesystem::path& path) noexcept -> std::filesystem::path {
      RETINA_PROFILE_SCOPED();
      return std::filesystem::path(RETINA_GUI_SHADER_DIRECTORY) / path;
    }

    RETINA_NODISCARD RETINA_INLINE auto WithFontPath(const std::filesystem::path& path) noexcept -> std::filesystem::path {
      RETINA_PROFILE_SCOPED();
      return std::filesystem::path(RETINA_GUI_FONT_DIRECTORY) / path;
    }

    RETINA_NODISCARD RETINA_INLINE auto ApplyEotf(const glm::vec4& color) noexcept -> glm::vec4 {
      RETINA_PROFILE_SCOPED();
      const auto rgb = glm::vec3(color);
      const auto cutoff = glm::lessThanEqual(rgb, glm::vec3(0.04045f));
      const auto lower = rgb / 12.92f;
      const auto higher = glm::pow((rgb + 0.055f) / 1.055f, glm::vec3(2.4f));
      return { glm::mix(higher, lower, cutoff), color.a };
    }

    auto SetLinearDarkStyle() noexcept -> void {
      auto& style = ImGui::GetStyle();
      ImGui::StyleColorsDark();
      auto* colors = style.Colors;
      for (auto i = 0; i < ImGuiCol_COUNT; ++i) {
        auto color = glm::vec4();
        std::memcpy(&color, &colors[i], sizeof(ImVec4));
        color = ApplyEotf(color);
        colors[i] = ImVec4(color.r, color.g, color.b, color.a);
      }
    }
  }

  CImGuiContext::CImGuiContext(WSI::CWindow& window, const Graphics::CDevice& device) noexcept
    : _window(window),
      _device(device.ToArcPtr())
  {
    RETINA_PROFILE_SCOPED();
  }

  CImGuiContext::~CImGuiContext() noexcept {
    RETINA_PROFILE_SCOPED();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  auto CImGuiContext::Make(
    WSI::CWindow& window,
    const Graphics::CDevice& device,
    const SImGuiContextCreateInfo& createInfo
  ) noexcept -> Core::CUniquePtr<CImGuiContext> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::MakeUnique<CImGuiContext>(window, device);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    Details::SetLinearDarkStyle();
    ImGui_ImplGlfw_InitForOther(static_cast<GLFWwindow*>(window.GetHandle()), true);
    {
      auto& io = ImGui::GetIO();
      io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard |
        ImGuiConfigFlags_DockingEnable;
      io.BackendRendererName = "Retina";
      io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

      io.Fonts->AddFontFromFileTTF(Details::WithFontPath("RobotoMono/RobotoMono_ItalicVariableFontWeight.ttf").generic_string().c_str(), 24.0f);
      io.FontDefault = io.Fonts->AddFontFromFileTTF(Details::WithFontPath("RobotoMono/RobotoMono_VariableFontWeight.ttf").generic_string().c_str(), 24.0f);
    }

    auto vertexBuffer = device.GetShaderResourceTable().MakeBuffer<SVertexFormat>(createInfo.MaxTimelineDifference, {
      .Name = "ImGuiContext_MainVertexBuffer",
      .Heap = Graphics::EHeapType::E_DEVICE_MAPPABLE,
      .Capacity = 1 << 20,
    });
    auto indexBuffer = device.GetShaderResourceTable().MakeBuffer<uint16>(createInfo.MaxTimelineDifference, {
      .Name = "ImGuiContext_MainIndexBuffer",
      .Heap = Graphics::EHeapType::E_DEVICE_MAPPABLE,
      .Capacity = 1 << 20,
    });
    auto pipeline = Graphics::CGraphicsPipeline::Make(device, {
      .Name = "ImGuiContext_MainPipeline",
      .VertexShader = Details::WithShaderPath("ImGui.vert.glsl"),
      .FragmentShader = Details::WithShaderPath("ImGui.frag.glsl"),
      .DescriptorLayouts = { device.GetShaderResourceTable().GetDescriptorLayout() },
      .ColorBlendState = {
        .Attachments = {
          {
            .BlendEnable = true,
          }
        }
      },
      .DynamicState = { {
        Graphics::EDynamicState::E_VIEWPORT,
        Graphics::EDynamicState::E_SCISSOR,
      } },
      .RenderingInfo = { {
        .ColorAttachmentFormats = {
          Graphics::EResourceFormat::E_R8G8B8A8_UNORM,
        },
      } }
    });

    auto fontSampler = device.GetShaderResourceTable().MakeSampler({
      .Name = "ImGuiContext_FontSampler",
      .Filter = { Graphics::EFilter::E_LINEAR },
      .Address = { Graphics::ESamplerAddressMode::E_REPEAT },
      .MipmapMode = Graphics::ESamplerMipmapMode::E_LINEAR,
    });

    auto fontTexture = [&] noexcept {
      auto& io = ImGui::GetIO();
      auto* pixels = static_cast<uint8*>(nullptr);
      auto width = 0_i32;
      auto height = 0_i32;
      io.FontAllowUserScaling = true;
      io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
      auto resource = device.GetShaderResourceTable().MakeImage({
        .Name = "ImGuiContext_FontTexture",
        .Width = static_cast<uint32>(width),
        .Height = static_cast<uint32>(height),
        .Format = Graphics::EResourceFormat::E_R8G8B8A8_UNORM,
        .Usage =
          Graphics::EImageUsageFlag::E_SAMPLED |
          Graphics::EImageUsageFlag::E_TRANSFER_DST,
        .ViewInfo = Graphics::DEFAULT_IMAGE_VIEW_CREATE_INFO,
      });
      auto staging = Graphics::CTypedBuffer<uint32>::Make(device, {
        .Name = "ImGuiContext_FontStagingBuffer",
        .Heap = Graphics::EHeapType::E_HOST_ONLY_CACHED,
        .Capacity = static_cast<uint32>(width * height),
      });
      staging->Write({
        reinterpret_cast<uint32*>(pixels),
        static_cast<usize>(width * height)
      });
      device.GetGraphicsQueue().Submit([&](Graphics::CCommandBuffer& commandBuffer) noexcept {
        commandBuffer
          .ImageMemoryBarrier({
            .Image = *resource,
            .SourceStage = Graphics::EPipelineStageFlag::E_TOP_OF_PIPE,
            .DestStage = Graphics::EPipelineStageFlag::E_TRANSFER,
            .SourceAccess = Graphics::EResourceAccessFlag::E_NONE,
            .DestAccess = Graphics::EResourceAccessFlag::E_TRANSFER_WRITE,
            .OldLayout = Graphics::EImageLayout::E_UNDEFINED,
            .NewLayout = Graphics::EImageLayout::E_TRANSFER_DST_OPTIMAL,
          })
          .CopyBufferToImage(*staging, *resource, {})
          .ImageMemoryBarrier({
            .Image = *resource,
            .SourceStage = Graphics::EPipelineStageFlag::E_TRANSFER,
            .DestStage = Graphics::EPipelineStageFlag::E_FRAGMENT_SHADER,
            .SourceAccess = Graphics::EResourceAccessFlag::E_TRANSFER_WRITE,
            .DestAccess = Graphics::EResourceAccessFlag::E_SHADER_SAMPLED_READ,
            .OldLayout = Graphics::EImageLayout::E_TRANSFER_DST_OPTIMAL,
            .NewLayout = Graphics::EImageLayout::E_SHADER_READ_ONLY_OPTIMAL,
          });
      });
      io.Fonts->SetTexID(nullptr);

      return resource;
    }();

    RETINA_GUI_INFO("Initialized ImGui context");

    self->_vertexBuffers = std::move(vertexBuffer);
    self->_indexBuffers = std::move(indexBuffer);
    self->_fontTexture = fontTexture;
    self->_fontSampler = fontSampler;
    self->_pipeline = std::move(pipeline);
    self->_createInfo = createInfo;
    return self;
  }

  auto CImGuiContext::NewFrame() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    _currentFrame = _frameCount++ % _createInfo.MaxTimelineDifference;
  }

  auto CImGuiContext::Render(const Graphics::CImage& target, Graphics::CCommandBuffer& commands) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    ImGui::Render();
    const auto* drawData = ImGui::GetDrawData();
    if (drawData && drawData->TotalVtxCount > 0) {
      auto& currentVertexBuffer = _vertexBuffers[_currentFrame];
      auto& currentIndexBuffer = _indexBuffers[_currentFrame];

      {
        auto currentVertexOffset = 0_u32;
        auto currentIndexOffset = 0_u32;
        for (auto i = 0_u32; i < drawData->CmdListsCount; ++i) {
          const auto& cmdList = *drawData->CmdLists[i];
          currentVertexBuffer->Write({
            reinterpret_cast<const SVertexFormat*>(cmdList.VtxBuffer.Data),
            static_cast<usize>(cmdList.VtxBuffer.Size)
          }, currentVertexOffset);
          currentIndexBuffer->Write({
            reinterpret_cast<const uint16*>(cmdList.IdxBuffer.Data),
            static_cast<usize>(cmdList.IdxBuffer.Size)
          }, currentIndexOffset);
          currentVertexOffset += cmdList.VtxBuffer.Size;
          currentIndexOffset += cmdList.IdxBuffer.Size;
        }
      }

      const auto clipOffset = drawData->DisplayPos;
      const auto clipScale = drawData->FramebufferScale;
      const auto targetWidth = static_cast<float32>(target.GetWidth());
      const auto targetHeight = static_cast<float32>(target.GetHeight());

      auto globalVertexOffset = 0_i32;
      auto globalIndexOffset = 0_i32;

      // Assumes the RT is already in the correct layout
      commands
        .BeginRendering({
          .Name = "ImGuiPass",
          .ColorAttachments = {
            {
              .ImageView = target.GetView(),
              .LoadOperator = Graphics::EAttachmentLoadOperator::E_LOAD,
              .StoreOperator = Graphics::EAttachmentStoreOperator::E_STORE,
            }
          }
        })
        .BindPipeline(*_pipeline)
        .BindShaderResourceTable(_device->GetShaderResourceTable())
        .SetViewport();
      for (auto i = 0_u32; i < drawData->CmdListsCount; ++i) {
        const auto& cmdList = *drawData->CmdLists[i];
        for (auto j = 0_u32; j < cmdList.CmdBuffer.Size; ++j) {
          const auto& cmd = cmdList.CmdBuffer[j];
          if (cmd.UserCallback) {
            if (cmd.UserCallback != ImDrawCallback_ResetRenderState) {
              cmd.UserCallback(&cmdList, &cmd);
            }
          } else {
            auto clipMin = ImVec2((cmd.ClipRect.x - clipOffset.x) * clipScale.x, (cmd.ClipRect.y - clipOffset.y) * clipScale.y);
            auto clipMax = ImVec2((cmd.ClipRect.z - clipOffset.x) * clipScale.x, (cmd.ClipRect.w - clipOffset.y) * clipScale.y);
            clipMin.x = glm::clamp(clipMin.x, 0.0f, targetWidth);
            clipMax.x = glm::clamp(clipMax.x, 0.0f, targetWidth);
            clipMin.y = glm::clamp(clipMin.y, 0.0f, targetHeight);
            clipMax.y = glm::clamp(clipMax.y, 0.0f, targetHeight);

            if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y) {
              continue;
            }

            auto scissor = Graphics::SScissor();
            scissor.X = static_cast<int32>(clipMin.x);
            scissor.Y = static_cast<int32>(clipMin.y);
            scissor.Width = static_cast<uint32>(clipMax.x - clipMin.x);
            scissor.Height = static_cast<uint32>(clipMax.y - clipMin.y);

            const auto textureId = cmd.TextureId
              ? reinterpret_cast<uint32>(cmd.TextureId)
              : _fontTexture.GetHandle();
            const auto samplerId = _fontSampler.GetHandle();
            const auto scale = glm::vec2(
              2.0f / drawData->DisplaySize.x,
              2.0f / drawData->DisplaySize.y
            );
            const auto translate = glm::vec2(
              -1.0f - drawData->DisplayPos.x * scale.x,
              -1.0f - drawData->DisplayPos.y * scale.y
            );
            commands
              .SetScissor(scissor)
              .BindIndexBuffer(*currentIndexBuffer, Graphics::EIndexType::E_UINT16)
              .PushConstants(
                currentVertexBuffer.GetHandle(),
                samplerId,
                textureId,
                scale,
                translate
              )
              .DrawIndexed(
                cmd.ElemCount,
                1,
                cmd.IdxOffset + globalIndexOffset,
                cmd.VtxOffset + globalVertexOffset,
                0
              );
          }
        }
        globalVertexOffset += cmdList.VtxBuffer.Size;
        globalIndexOffset += cmdList.IdxBuffer.Size;
      }
      commands.EndRendering();
    }
  }
}
