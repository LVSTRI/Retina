#pragma once

#include <Retina/Graphics/CommandBufferInfo.hpp>

#include <vulkan/vulkan.h>

#include <vector>
#include <span>

namespace Retina::Graphics {
  class CCommandBuffer : public Core::IEnableIntrusiveReferenceCount<CCommandBuffer> {
  public:
    CCommandBuffer() noexcept = default;
    ~CCommandBuffer() noexcept;

    RETINA_NODISCARD static auto Make(
      const CQueue& queue,
      const SCommandBufferCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CCommandBuffer>;

    RETINA_NODISCARD static auto Make(
      const CQueue& queue,
      uint32 count,
      const SCommandBufferCreateInfo& createInfo
    ) noexcept -> std::vector<Core::CArcPtr<CCommandBuffer>>;

    RETINA_NODISCARD static auto MakeWith(
      CCommandPool& commandPool,
      const SCommandBufferCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CCommandBuffer>;

    RETINA_NODISCARD static auto MakeWith(
      CCommandPool& commandPool,
      uint32 count,
      const SCommandBufferCreateInfo& createInfo
    ) noexcept -> std::vector<Core::CArcPtr<CCommandBuffer>>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkCommandBuffer;
    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SCommandBufferCreateInfo&;
    RETINA_NODISCARD auto GetCommandPool() const noexcept -> CCommandPool&;
    RETINA_NODISCARD auto GetQueue() const noexcept -> const CQueue&;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

    auto Begin() noexcept -> CCommandBuffer&;
    auto End() noexcept -> CCommandBuffer&;

    auto BeginRendering(const SRenderingInfo& renderingInfo) noexcept -> CCommandBuffer&;
    auto EndRendering() noexcept -> CCommandBuffer&;

    auto Draw(
      uint32 vertexCount,
      uint32 instanceCount = 1,
      uint32 firstVertex = 0,
      uint32 firstInstance = 0
    ) noexcept -> CCommandBuffer&;

    auto Barrier(const SMemoryBarrierInfo& barrierInfo) noexcept -> CCommandBuffer&;
    auto MemoryBarrier(const SMemoryBarrier& barrier) noexcept -> CCommandBuffer&;
    auto ImageMemoryBarrier(const SImageMemoryBarrier& barrier) noexcept -> CCommandBuffer&;

    auto CopyImage(const CImage& source, const CImage& dest, SImageCopyRegion copyRegion) noexcept -> CCommandBuffer&;
    auto BlitImage(const CImage& source, const CImage& dest, SImageBlitRegion blitRegion) noexcept -> CCommandBuffer&;

    auto BeginNamedRegion(std::string_view name) noexcept -> CCommandBuffer&;
    auto EndNamedRegion() noexcept -> CCommandBuffer&;

  private:
    VkCommandBuffer _handle = {};
    SCommandBufferCreateInfo _createInfo = {};
    Core::CArcPtr<CCommandPool> _commandPool;
    Core::CArcPtr<const CQueue> _queue;
  };
}
