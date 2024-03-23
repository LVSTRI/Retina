#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Enum.hpp>
#include <Retina/Graphics/QueueInfo.hpp>

#include <compare>
#include <optional>
#include <string>

namespace Retina::Graphics {
  struct SImageSubresource {
    uint32 BaseLevel = 0;
    uint32 BaseLayer = 0;

    RETINA_NODISCARD constexpr auto operator <=>(const SImageSubresource&) const noexcept = default;
  };

  struct SImageSubresourceRange {
    uint32 BaseLevel = SUBRESOURCE_LEVEL_IGNORED;
    uint32 LevelCount = SUBRESOURCE_REMAINING_LEVELS;
    uint32 BaseLayer = SUBRESOURCE_LAYER_IGNORED;
    uint32 LayerCount = SUBRESOURCE_REMAINING_LAYERS;

    RETINA_NODISCARD constexpr auto operator <=>(const SImageSubresourceRange&) const noexcept = default;
  };

  struct SOffset2D {
    int32 X = 0;
    int32 Y = 0;

    RETINA_NODISCARD constexpr auto operator <=>(const SOffset2D&) const noexcept = default;
  };

  struct SExtent2D {
    uint32 Width = 0;
    uint32 Height = 0;

    RETINA_NODISCARD constexpr auto operator <=>(const SExtent2D&) const noexcept = default;
  };

  struct SRect2D {
    SOffset2D Offset = {};
    SExtent2D Extent = {};

    RETINA_NODISCARD constexpr auto operator <=>(const SRect2D&) const noexcept = default;
  };

  struct SOffset3D {
    int32 X = 0;
    int32 Y = 0;
    int32 Z = 0;

    RETINA_NODISCARD constexpr auto operator <=>(const SOffset3D&) const noexcept = default;
  };

  struct SExtent3D {
    uint32 Width = 0;
    uint32 Height = 0;
    uint32 Depth = 0;

    RETINA_NODISCARD constexpr auto operator <=>(const SExtent3D&) const noexcept = default;
  };

  struct SRect3D {
    SOffset3D Offset = {};
    SExtent3D Extent = {};

    RETINA_NODISCARD constexpr auto operator <=>(const SRect3D&) const noexcept = default;
  };

  struct SImageSwizzle {
    EComponentSwizzle R = EComponentSwizzle::E_IDENTITY;
    EComponentSwizzle G = EComponentSwizzle::E_IDENTITY;
    EComponentSwizzle B = EComponentSwizzle::E_IDENTITY;
    EComponentSwizzle A = EComponentSwizzle::E_IDENTITY;

    RETINA_NODISCARD constexpr auto operator <=>(const SImageSwizzle&) const noexcept = default;
  };

  struct SImageViewCreateInfo {
    std::string Name;
    EResourceFormat Format = {};
    SImageSwizzle Swizzle = {};
    SImageSubresourceRange SubresourceRange = {};
  };

  const inline auto DEFAULT_IMAGE_VIEW_CREATE_INFO = SImageViewCreateInfo();

  struct SImageCreateInfo {
    std::string Name;
    EImageCreateFlag Flags = {};
    uint32 Width = 0;
    uint32 Height = 0;
    uint32 Levels = 1;
    uint32 Layers = 1;
    EResourceFormat Format = {};
    ESampleCountFlag Samples = ESampleCountFlag::E_1;
    EImageUsageFlag Usage = {};
    EQueueDomain Domain = EQueueDomain::E_GRAPHICS;
    bool IsCrossDomain = false;
    bool IsDedicated = false;
    std::optional<SImageViewCreateInfo> ViewInfo = std::nullopt;
  };

  RETINA_NODISCARD RETINA_INLINE constexpr auto ImageAspectMaskFrom(EResourceFormat format) noexcept -> EImageAspectFlag {
    switch (format) {
      case EResourceFormat::E_S8_UINT:
        return EImageAspectFlag::E_STENCIL;
      case EResourceFormat::E_D16_UNORM:
      case EResourceFormat::E_X8_D24_UNORM_PACK32:
      case EResourceFormat::E_D32_SFLOAT:
        return EImageAspectFlag::E_DEPTH;
      case EResourceFormat::E_D16_UNORM_S8_UINT:
      case EResourceFormat::E_D24_UNORM_S8_UINT:
      case EResourceFormat::E_D32_SFLOAT_S8_UINT:
        return EImageAspectFlag::E_DEPTH | EImageAspectFlag::E_STENCIL;
      default:
        return EImageAspectFlag::E_COLOR;
    }
  }

  RETINA_NODISCARD RETINA_INLINE constexpr auto MakeNativeImageSubresourceRange(
    const EImageAspectFlag aspectMask,
    const SImageSubresourceRange& subresource
  ) noexcept -> VkImageSubresourceRange {
    auto subresourceRange = VkImageSubresourceRange();
    subresourceRange.aspectMask = AsEnumCounterpart(aspectMask);
    subresourceRange.baseMipLevel = subresource.BaseLevel;
    if (subresourceRange.baseMipLevel == SUBRESOURCE_LEVEL_IGNORED) {
      subresourceRange.baseMipLevel = 0;
    }
    subresourceRange.baseArrayLayer = subresource.BaseLayer;
    if (subresourceRange.baseArrayLayer == SUBRESOURCE_LAYER_IGNORED) {
      subresourceRange.baseArrayLayer = 0;
    }
    subresourceRange.levelCount = subresource.LevelCount;
    subresourceRange.layerCount = subresource.LayerCount;
    return subresourceRange;
  }

  RETINA_NODISCARD auto MakeNativeImageSubresourceRangeFrom(const CImage& image) noexcept -> VkImageSubresourceRange;

  RETINA_NODISCARD auto MakeNativeImageSubresourceRange(
    const CImage& image,
    const SImageSubresourceRange& subresource
  ) noexcept -> VkImageSubresourceRange;

  RETINA_NODISCARD auto MakeNativeImageSubresourceLayers(
    const CImage& image,
    const SImageSubresourceRange& subresourceRange
  ) noexcept -> VkImageSubresourceLayers;
}
