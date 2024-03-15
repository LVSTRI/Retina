#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/BufferInfo.hpp>
#include <Retina/Graphics/CommandPoolInfo.hpp>
#include <Retina/Graphics/ImageInfo.hpp>

#include <optional>
#include <array>

namespace Retina::Graphics {
  struct SDrawIndirectCommand {
    uint32 VertexCount = 0;
    uint32 InstanceCount = 0;
    uint32 FirstVertex = 0;
    uint32 FirstInstance = 0;
  };

  struct SDrawIndexedIndirectCommand {
    uint32 IndexCount = 0;
    uint32 InstanceCount = 0;
    uint32 FirstIndex = 0;
    int32 VertexOffset = 0;
    uint32 FirstInstance = 0;
  };

  struct SDrawMeshTasksIndirectCommand {
    uint32 X = 0;
    uint32 Y = 0;
    uint32 Z = 0;
  };

  struct SDispatchIndirectCommand {
    uint32 X = 0;
    uint32 Y = 0;
    uint32 Z = 0;
  };

  struct SMemoryBarrier {
    EPipelineStageFlag SourceStage = EPipelineStageFlag::E_ALL_COMMANDS;
    EPipelineStageFlag DestStage = EPipelineStageFlag::E_ALL_COMMANDS;
    EResourceAccessFlag SourceAccess = EResourceAccessFlag::E_MEMORY_WRITE;
    EResourceAccessFlag DestAccess = EResourceAccessFlag::E_MEMORY_WRITE | EResourceAccessFlag::E_MEMORY_READ;
  };

  struct SBufferMemoryBarrier {
    Core::CReferenceWrapper<const CBuffer> Buffer;
    EPipelineStageFlag SourceStage = EPipelineStageFlag::E_ALL_COMMANDS;
    EPipelineStageFlag DestStage = EPipelineStageFlag::E_ALL_COMMANDS;
    EResourceAccessFlag SourceAccess = EResourceAccessFlag::E_MEMORY_WRITE;
    EResourceAccessFlag DestAccess = EResourceAccessFlag::E_MEMORY_WRITE | EResourceAccessFlag::E_MEMORY_READ;
    SBufferMemoryRange MemoryRange = {};
  };

  struct SImageMemoryBarrier {
    Core::CReferenceWrapper<const CImage> Image;
    EPipelineStageFlag SourceStage = EPipelineStageFlag::E_ALL_COMMANDS;
    EPipelineStageFlag DestStage = EPipelineStageFlag::E_ALL_COMMANDS;
    EResourceAccessFlag SourceAccess = EResourceAccessFlag::E_MEMORY_WRITE;
    EResourceAccessFlag DestAccess = EResourceAccessFlag::E_MEMORY_WRITE | EResourceAccessFlag::E_MEMORY_READ;
    EImageLayout OldLayout = EImageLayout::E_UNDEFINED;
    EImageLayout NewLayout = EImageLayout::E_UNDEFINED;
    SImageSubresourceRange SubresourceRange = {};
  };

  struct SMemoryBarrierInfo {
    std::vector<SMemoryBarrier> MemoryBarriers;
    std::vector<SBufferMemoryBarrier> BufferMemoryBarriers;
    std::vector<SImageMemoryBarrier> ImageMemoryBarriers;
  };

  struct SBufferCopyRegion {
    usize SourceOffset = 0;
    usize DestOffset = 0;
    usize Size = WHOLE_SIZE;
  };

  struct SImageCopyRegion {
    SOffset3D SourceOffset = {};
    SOffset3D DestOffset = {};
    SImageSubresourceRange SourceSubresource = {};
    SImageSubresourceRange DestSubresource = {};
    SExtent3D Extent = {};
  };

  struct SImageBlitRegion {
    std::array<SOffset3D, 2> SourceOffsets = {};
    std::array<SOffset3D, 2> DestOffsets = {};
    SImageSubresourceRange SourceSubresource = {};
    SImageSubresourceRange DestSubresource = {};
    EFilter Filter = EFilter::E_NEAREST;
  };

  union SColorClearValue {
    float32 Float32[4];
    int32 Int32[4];
    uint32 Uint32[4] = {};
  };

  struct SDepthStencilClearValue {
    float32 Depth = 1.0f;
    uint32 Stencil = 0;
  };

  union SClearValue {
    SColorClearValue Color = {};
    SDepthStencilClearValue DepthStencil;
  };

  struct SAttachmentInfo {
    Core::CReferenceWrapper<const CImageView> ImageView;
    EAttachmentLoadOperator LoadOperator = EAttachmentLoadOperator::E_CLEAR;
    EAttachmentStoreOperator StoreOperator = EAttachmentStoreOperator::E_STORE;
    SClearValue ClearValue = {};
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

  struct SCommandBufferCreateInfo {
    std::string Name;
    ECommandBufferLevel Level = ECommandBufferLevel::E_PRIMARY;
    std::optional<SCommandPoolCreateInfo> PoolInfo = std::nullopt;
  };

  RETINA_NODISCARD RETINA_INLINE constexpr auto MakeColorClearValue(const float32(&array)[4] = {}) noexcept -> SClearValue {
    return {
      .Color = {
        .Float32 = {
          array[0],
          array[1],
          array[2],
          array[3]
        }
      }
    };
  }

  RETINA_NODISCARD RETINA_INLINE constexpr auto MakeColorClearValue(const int32(&array)[4] = {}) noexcept -> SClearValue {
    return {
      .Color = {
        .Int32 = {
          array[0],
          array[1],
          array[2],
          array[3]
        }
      }
    };
  }

  RETINA_NODISCARD RETINA_INLINE constexpr auto MakeColorClearValue(const uint32(&array)[4] = {}) noexcept -> SClearValue {
    return {
      .Color = {
        .Uint32 = {
          array[0],
          array[1],
          array[2],
          array[3]
        }
      }
    };
  }

  RETINA_NODISCARD RETINA_INLINE constexpr auto MakeColorClearValue(float32 value) noexcept -> SClearValue {
    return {
      .Color = {
        .Float32 = {
          value,
          value,
          value,
          value
        }
      }
    };
  }

  RETINA_NODISCARD RETINA_INLINE constexpr auto MakeColorClearValue(int32 value) noexcept -> SClearValue {
    return {
      .Color = {
        .Int32 = {
          value,
          value,
          value,
          value
        }
      }
    };
  }

  RETINA_NODISCARD RETINA_INLINE constexpr auto MakeColorClearValue(uint32 value) noexcept -> SClearValue {
    return {
      .Color = {
        .Uint32 = {
          value,
          value,
          value,
          value
        }
      }
    };
  }

  RETINA_NODISCARD RETINA_INLINE constexpr auto MakeDepthStencilClearValue(float32 depth = 0.0f, uint32 stencil = 0) noexcept -> SClearValue {
    return {
      .DepthStencil = {
        .Depth = depth,
        .Stencil = stencil
      }
    };
  }
}
