#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/ImageInfo.hpp>
#include <Retina/Graphics/Forward.hpp>

#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>

struct NVSDK_NGX_Parameter;
struct NVSDK_NGX_Handle;

namespace Retina::Graphics {
  enum class ENvidiaDlssQualityPreset {
    E_PERFORMANCE,
    E_BALANCED,
    E_QUALITY,
    E_NATIVE,
  };

  struct SNvidiaDlssInitializeInfo {
    SExtent2D RenderResolution = {};
    SExtent2D OutputResolution = {};
    ENvidiaDlssQualityPreset Quality = ENvidiaDlssQualityPreset::E_NATIVE;
    float32 DepthScale = 1.0f;

    bool IsHDR = false;
    bool IsReverseDepth = false;
    bool EnableAutoExposure = false;
  };

  struct SNvidiaDlssEvaluateInfo {
    Core::CReferenceWrapper<const CImage> Color;
    Core::CReferenceWrapper<const CImage> Depth;
    Core::CReferenceWrapper<const CImage> Velocity;
    Core::CReferenceWrapper<const CImage> Output;
    glm::vec2 JitterOffset = {};
    glm::vec2 MotionVectorScale = { 1.0f, 1.0f };
    bool Reset = false;
  };

  class CNvidiaDlssFeature {
  public:
    CNvidiaDlssFeature() noexcept = default;
    ~CNvidiaDlssFeature() noexcept;

    RETINA_NODISCARD static auto Make(const CDevice& device) noexcept -> Core::CUniquePtr<CNvidiaDlssFeature>;

    auto Initialize(const SNvidiaDlssInitializeInfo& initializeInfo) noexcept -> void;
    auto Shutdown() noexcept -> void;

    auto Evaluate(CCommandBuffer& commands, const SNvidiaDlssEvaluateInfo& evaluateInfo) noexcept -> void;

  private:
    NVSDK_NGX_Parameter* _parameters = nullptr;
    NVSDK_NGX_Handle* _handle = nullptr;

    Core::CArcPtr<const CDevice> _device;
  };

  RETINA_NODISCARD auto GetNvidiaDlssIstanceExtensions() noexcept -> std::vector<const char*>;
  RETINA_NODISCARD auto GetNvidiaDlssDeviceExtensions(VkInstance instance, VkPhysicalDevice physicalDevice) noexcept -> std::vector<const char*>;

  RETINA_NODISCARD RETINA_INLINE constexpr auto GetScalingRatioFromQualityPreset(ENvidiaDlssQualityPreset quality) noexcept -> float32 {
    switch (quality) {
      case ENvidiaDlssQualityPreset::E_PERFORMANCE: return 0.5f;
      case ENvidiaDlssQualityPreset::E_BALANCED: return 0.58f;
      case ENvidiaDlssQualityPreset::E_QUALITY: return 0.67f;
      case ENvidiaDlssQualityPreset::E_NATIVE: return 1.0f;
      default: std::unreachable();
    }
  }
}
