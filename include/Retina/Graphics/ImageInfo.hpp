#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/QueueInfo.hpp>

#include <optional>

namespace Retina {
    struct SImageSubresource {
        uint32 BaseLevel = 0;
        uint32 BaseLayer = 0;

        RETINA_NODISCARD constexpr auto operator <=>(const SImageSubresource&) const noexcept -> std::strong_ordering = default;
    };

    struct SImageSubresourceRange {
        uint32 BaseLevel = Constant::SUBRESOURCE_LEVEL_IGNORED;
        uint32 BaseLayer = Constant::SUBRESOURCE_LAYER_IGNORED;
        uint32 LevelCount = Constant::SUBRESOURCE_REMAINING_LEVELS;
        uint32 LayerCount = Constant::SUBRESOURCE_REMAINING_LAYERS;

        RETINA_NODISCARD constexpr auto operator <=>(const SImageSubresourceRange&) const noexcept -> std::strong_ordering = default;
    };

    struct SOffset2D {
        int32 X = 0;
        int32 Y = 0;

        RETINA_NODISCARD constexpr auto operator <=>(const SOffset2D&) const noexcept -> std::strong_ordering = default;
    };

    struct SExtent2D {
        uint32 Width = 0;
        uint32 Height = 0;

        RETINA_NODISCARD constexpr auto operator <=>(const SExtent2D&) const noexcept -> std::strong_ordering = default;
    };

    struct SRect2D {
        SOffset2D Offset = {};
        SExtent2D Extent = {};

        RETINA_NODISCARD constexpr auto operator <=>(const SRect2D&) const noexcept -> std::strong_ordering = default;
    };

    struct SOffset3D {
        int32 X = 0;
        int32 Y = 0;
        int32 Z = 0;

        RETINA_NODISCARD constexpr auto operator <=>(const SOffset3D&) const noexcept -> std::strong_ordering = default;
    };

    struct SExtent3D {
        uint32 Width = 0;
        uint32 Height = 0;
        uint32 Depth = 0;

        RETINA_NODISCARD constexpr auto operator <=>(const SExtent3D&) const noexcept -> std::strong_ordering = default;
    };

    struct SRect3D {
        SOffset3D Offset = {};
        SExtent3D Extent = {};

        RETINA_NODISCARD constexpr auto operator <=>(const SRect3D&) const noexcept -> std::strong_ordering = default;
    };

    struct SImageSwizzle {
        EComponentSwizzle R = EComponentSwizzle::E_IDENTITY;
        EComponentSwizzle G = EComponentSwizzle::E_IDENTITY;
        EComponentSwizzle B = EComponentSwizzle::E_IDENTITY;
        EComponentSwizzle A = EComponentSwizzle::E_IDENTITY;

        RETINA_NODISCARD constexpr auto operator <=>(const SImageSwizzle&) const noexcept -> std::strong_ordering = default;
    };

    struct SImageViewCreateInfo {
        std::string Name;
        EResourceFormat Format = EResourceFormat::E_UNDEFINED;
        SImageSwizzle Swizzle = {};
        SImageSubresourceRange Subresource = {};

        RETINA_NODISCARD constexpr auto operator <=>(const SImageViewCreateInfo&) const noexcept -> std::strong_ordering = default;
    };

    struct SImageCreateInfo {
        std::string Name;
        uint32 Width = 0;
        uint32 Height = 0;
        uint32 Levels = 1;
        uint32 Layers = 1;
        bool IsCrossDomain = false;
        EQueueDomain QueueDomain = EQueueDomain::E_GRAPHICS;
        ESampleCount Samples = ESampleCount::E_1;
        EImageUsage Usage = {};
        EImageCreateFlag Flags = {};
        EResourceFormat Format = {};
        std::optional<SImageViewCreateInfo> ViewInfo = std::nullopt;

        RETINA_NODISCARD constexpr auto operator <=>(const SImageCreateInfo&) const noexcept -> std::strong_ordering = default;
    };

    namespace Constant {
        const inline auto DEFAULT_IMAGE_VIEW_INFO = SImageViewCreateInfo();
    }

    RETINA_NODISCARD constexpr auto ImageAspectMaskFromFormat(EResourceFormat format) noexcept -> EImageAspect {
        switch (format) {
            case EResourceFormat::E_S8_UINT:
                return EImageAspect::E_STENCIL;
            case EResourceFormat::E_D16_UNORM:
            case EResourceFormat::E_X8_D24_UNORM_PACK32:
            case EResourceFormat::E_D32_SFLOAT:
                return EImageAspect::E_DEPTH;
            case EResourceFormat::E_D16_UNORM_S8_UINT:
            case EResourceFormat::E_D24_UNORM_S8_UINT:
            case EResourceFormat::E_D32_SFLOAT_S8_UINT:
                return EImageAspect::E_DEPTH | EImageAspect::E_STENCIL;
            default:
                return EImageAspect::E_COLOR;
        }
    }

    RETINA_NODISCARD constexpr auto MakeNativeImageSubresourceRange(
        const EImageAspect aspectMask,
        const SImageSubresourceRange& subresource
    ) noexcept -> VkImageSubresourceRange {
        auto subresourceRange = VkImageSubresourceRange();
        subresourceRange.aspectMask = ToEnumCounterpart(aspectMask);
        subresourceRange.baseMipLevel = subresource.BaseLevel;
        if (subresourceRange.baseMipLevel == Constant::SUBRESOURCE_LEVEL_IGNORED) {
            subresourceRange.baseMipLevel = 0;
        }
        subresourceRange.levelCount = subresource.LevelCount;
        if (subresourceRange.levelCount == Constant::SUBRESOURCE_REMAINING_LEVELS) {
            subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        }
        subresourceRange.baseArrayLayer = subresource.BaseLayer;
        if (subresourceRange.baseArrayLayer == Constant::SUBRESOURCE_LAYER_IGNORED) {
            subresourceRange.baseArrayLayer = 0;
        }
        subresourceRange.layerCount = subresource.LayerCount;
        if (subresourceRange.layerCount == Constant::SUBRESOURCE_REMAINING_LAYERS) {
            subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        }
        return subresourceRange;
    }

    RETINA_NODISCARD auto MakeNativeImageSubresourceLayers(
        const CImage& image,
        const SImageSubresourceRange& subresource
    ) noexcept -> VkImageSubresourceLayers;
 }
