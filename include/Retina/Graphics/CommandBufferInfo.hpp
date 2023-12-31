#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/CommandPoolInfo.hpp>
#include <Retina/Graphics/ImageInfo.hpp>

#include <optional>
#include <array>

namespace Retina {
    struct SDrawIndirectCommand {
        uint32 VertexCount = 0;
        uint32 InstanceCount = 0;
        uint32 FirstVertex = 0;
        uint32 FirstInstance = 0;

        RETINA_NODISCARD constexpr auto operator <=>(const SDrawIndirectCommand&) const noexcept -> std::strong_ordering = default;
    };

    struct SDrawIndexedIndirectCommand {
        uint32 IndexCount = 0;
        uint32 InstanceCount = 0;
        uint32 FirstIndex = 0;
        int32 VertexOffset = 0;
        uint32 FirstInstance = 0;

        RETINA_NODISCARD constexpr auto operator <=>(const SDrawIndexedIndirectCommand&) const noexcept -> std::strong_ordering = default;
    };

    struct SDrawMeshTasksIndirectCommand {
        uint32 X = 0;
        uint32 Y = 0;
        uint32 Z = 0;

        RETINA_NODISCARD constexpr auto operator <=>(const SDrawMeshTasksIndirectCommand&) const noexcept -> std::strong_ordering = default;
    };

    struct SDispatchIndirectCommand {
        uint32 X = 0;
        uint32 Y = 0;
        uint32 Z = 0;

        RETINA_NODISCARD constexpr auto operator <=>(const SDispatchIndirectCommand&) const noexcept -> std::strong_ordering = default;
    };

    struct SMemoryBarrier {
        EPipelineStage SourceStage = EPipelineStage::E_ALL_COMMANDS;
        EPipelineStage DestStage = EPipelineStage::E_ALL_COMMANDS;
        EResourceAccess SourceAccess = EResourceAccess::E_MEMORY_READ | EResourceAccess::E_MEMORY_WRITE;
        EResourceAccess DestAccess = EResourceAccess::E_MEMORY_READ | EResourceAccess::E_MEMORY_WRITE;

        RETINA_NODISCARD constexpr auto operator <=>(const SMemoryBarrier&) const noexcept -> std::strong_ordering = default;
    };

    struct SBufferMemoryBarrier {
        std::reference_wrapper<const CBuffer> Buffer;
        EPipelineStage SourceStage = EPipelineStage::E_ALL_COMMANDS;
        EPipelineStage DestStage = EPipelineStage::E_ALL_COMMANDS;
        EResourceAccess SourceAccess = EResourceAccess::E_MEMORY_READ | EResourceAccess::E_MEMORY_WRITE;
        EResourceAccess DestAccess = EResourceAccess::E_MEMORY_READ | EResourceAccess::E_MEMORY_WRITE;
        uint64 Offset = 0;
        uint64 Size = -1_u64;
    };

    struct SImageMemoryBarrier {
        std::reference_wrapper<const CImage> Image;
        EPipelineStage SourceStage = EPipelineStage::E_ALL_COMMANDS;
        EPipelineStage DestStage = EPipelineStage::E_ALL_COMMANDS;
        EResourceAccess SourceAccess = EResourceAccess::E_MEMORY_READ | EResourceAccess::E_MEMORY_WRITE;
        EResourceAccess DestAccess = EResourceAccess::E_MEMORY_READ | EResourceAccess::E_MEMORY_WRITE;
        EImageLayout OldLayout = EImageLayout::E_UNDEFINED;
        EImageLayout NewLayout = EImageLayout::E_UNDEFINED;
        SImageSubresourceRange Subresource = {};
    };

    struct SBufferCopyRegion {
        uint64 SourceOffset = 0;
        uint64 DestOffset = 0;
        uint64 Size = Constant::WHOLE_SIZE;

        RETINA_NODISCARD constexpr auto operator <=>(const SBufferCopyRegion&) const noexcept -> std::strong_ordering = default;
    };

    struct SImageCopyRegion {
        SOffset3D SourceOffset = {};
        SOffset3D DestOffset = {};
        SImageSubresourceRange SourceSubresource = {};
        SImageSubresourceRange DestSubresource = {};
        SExtent3D Extent = {};

        RETINA_NODISCARD constexpr auto operator <=>(const SImageCopyRegion&) const noexcept -> std::strong_ordering = default;
    };

    struct SImageBlitRegion {
        std::array<SOffset3D, 2> SourceOffsets = {};
        std::array<SOffset3D, 2> DestOffsets = {};
        SImageSubresourceRange SourceSubresource = {};
        SImageSubresourceRange DestSubresource = {};
        ESamplerFilter SamplerFilter = ESamplerFilter::E_NEAREST;

        RETINA_NODISCARD constexpr auto operator <=>(const SImageBlitRegion&) const noexcept -> std::strong_ordering = default;
    };

    union SColorClearValue {
        float32 Float32[4];
        int32 Int32[4];
        uint32 Uint32[4] = {};
    };

    struct SDepthStencilClearValue {
        float32 Depth = 0.0f;
        uint32 Stencil = 0;
    };

    union SAttachmentClearValue {
        SColorClearValue Color = {};
        SDepthStencilClearValue DepthStencil;
    };

    struct SAttachmentInfo {
        std::reference_wrapper<const CImage> Image;
        EAttachmentLoadOperator LoadOperation = EAttachmentLoadOperator::E_CLEAR;
        EAttachmentStoreOperator StoreOperation = EAttachmentStoreOperator::E_STORE;
        SAttachmentClearValue ClearValue = {};
    };

    struct SRenderingInfo {
        std::string Name;
        SRect2D RenderArea = {};
        uint32 LayerCount = -1_u32;
        uint32 ViewMask = 0;
        std::vector<SAttachmentInfo> ColorAttachments;
        std::optional<SAttachmentInfo> DepthAttachment = std::nullopt;
        std::optional<SAttachmentInfo> StencilAttachment = std::nullopt;
    };

    struct SAccelerationStructureCopyInfo {
        VkAccelerationStructureKHR Source = {};
        VkAccelerationStructureKHR Dest = {};
        EAccelerationStructureCopyMode Mode = {};
    };

    struct SCommandBufferCreateInfo {
        std::string Name;
        bool Primary = true;
        std::optional<SCommandPoolCreateInfo> CommandPoolInfo = std::nullopt;

        RETINA_NODISCARD constexpr auto operator <=>(const SCommandBufferCreateInfo&) const noexcept -> std::strong_ordering = default;
    };

    RETINA_NODISCARD constexpr auto MakeClearColorValue(const float32(&values)[4] = {}) noexcept -> SAttachmentClearValue {
        return {
            .Color = {
                .Float32 = {
                    values[0],
                    values[1],
                    values[2],
                    values[3]
                }
            }
        };
    }

    RETINA_NODISCARD constexpr auto MakeClearColorValue(const int32(&values)[4] = {}) noexcept -> SAttachmentClearValue {
        return {
            .Color = {
                .Int32 = {
                    values[0],
                    values[1],
                    values[2],
                    values[3]
                }
            }
        };
    }

    RETINA_NODISCARD constexpr auto MakeClearColorValue(const uint32(&values)[4] = {}) noexcept -> SAttachmentClearValue {
        return {
            .Color = {
                .Uint32 = {
                    values[0],
                    values[1],
                    values[2],
                    values[3]
                }
            }
        };
    }

    RETINA_NODISCARD constexpr auto MakeClearDepthStencilValue(float32 depth = 0.0f, uint32 stencil = 0) noexcept -> SAttachmentClearValue {
        return {
            .DepthStencil = {
                .Depth = depth,
                .Stencil = stencil
            }
        };
    }
}
