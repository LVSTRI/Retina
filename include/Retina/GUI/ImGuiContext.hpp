#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Resources/ShaderResource.hpp>
#include <Retina/Graphics/TypedBuffer.hpp>
#include <Retina/Graphics/Forward.hpp>

#include <Retina/WSI/Forward.hpp>

#include <glm/glm.hpp>

#include <imgui.h>

namespace Retina::GUI {
  struct SImGuiContextCreateInfo {
    uint32 MaxTimelineDifference = -1_u32;
  };

  class CImGuiContext {
  public:
    CImGuiContext(WSI::CWindow& window, const Graphics::CDevice& device) noexcept;
    ~CImGuiContext() noexcept;

    RETINA_NODISCARD static auto Make(
      WSI::CWindow& window,
      const Graphics::CDevice& device,
      const SImGuiContextCreateInfo& createInfo
    ) noexcept -> Core::CUniquePtr<CImGuiContext>;

    auto NewFrame() noexcept -> void;
    template <typename F>
    auto Render(const Graphics::CImage& target, Graphics::CCommandBuffer& commands, F&& f) noexcept -> void;

  private:
    struct SVertexFormat {
      glm::vec2 Position = {};
      glm::vec2 Uv = {};
      uint32 Color = {};
    };
    static_assert(sizeof(SVertexFormat) == 20, "Invalid vertex format size");

  private:
    auto Render(const Graphics::CImage& target, Graphics::CCommandBuffer& commands) noexcept -> void;

  private:
    std::vector<Graphics::CShaderResource<Graphics::CTypedBuffer<SVertexFormat>>> _vertexBuffers;
    std::vector<Graphics::CShaderResource<Graphics::CTypedBuffer<uint16>>> _indexBuffers;
    Graphics::CShaderResource<Graphics::CImage> _fontTexture;
    Graphics::CShaderResource<Graphics::CSampler> _fontSampler;
    Core::CArcPtr<Graphics::CGraphicsPipeline> _pipeline;

    uint32 _currentFrame = 0;
    uint32 _frameCount = 0;

    SImGuiContextCreateInfo _createInfo;
    Core::CReferenceWrapper<WSI::CWindow> _window;
    Core::CArcPtr<const Graphics::CDevice> _device;
  };

  template <typename F>
  auto CImGuiContext::Render(const Graphics::CImage& target, Graphics::CCommandBuffer& commands, F&& f) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    f();
    Render(target, commands);
  }

  template <typename T>
    requires
      std::same_as<T, Graphics::CImage> ||
      std::same_as<T, Graphics::CImageView>
  RETINA_NODISCARD RETINA_INLINE constexpr auto AsTextureHandle(Graphics::CShaderResource<T> resource) noexcept -> ImTextureID {
    return reinterpret_cast<ImTextureID>(resource.GetHandle());
  }
}
