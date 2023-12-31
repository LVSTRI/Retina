#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/RayTracing/AccelerationStructureInfo.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/CommandBufferInfo.hpp>
#include <Retina/Graphics/PipelineInfo.hpp>

#include <vulkan/vulkan.h>

#include <span>

namespace Retina {
    namespace Private {
        struct SRenderingInfoState {
            SRect2D RenderArea = {};
        };

        struct SPipelineInfoState {
            const IPipeline* Pipeline = nullptr;
        };
    }

    class CCommandBuffer : public INativeDebugName, public IEnableIntrusiveReferenceCount<CCommandBuffer> {
    public:
        using Self = CCommandBuffer;

        CCommandBuffer() noexcept = default;
        ~CCommandBuffer() noexcept;

        RETINA_NODISCARD static auto Make(const CQueue& queue, const SCommandBufferCreateInfo& createInfo) noexcept -> CArcPtr<Self>;
        RETINA_NODISCARD static auto MakeWith(const CCommandPool& pool, const SCommandBufferCreateInfo& createInfo) noexcept -> CArcPtr<Self>;
        RETINA_NODISCARD static auto Make(
            const CQueue& queue,
            uint32 count,
            const SCommandBufferCreateInfo& createInfo
        ) noexcept -> std::vector<CArcPtr<Self>>;
        RETINA_NODISCARD static auto MakeWith(
            const CCommandPool& pool,
            uint32 count,
            const SCommandBufferCreateInfo& createInfo
        ) noexcept -> std::vector<CArcPtr<Self>>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkCommandBuffer;
        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SCommandBufferCreateInfo&;
        RETINA_NODISCARD auto GetCommandPool() const noexcept -> const CCommandPool&;
        RETINA_NODISCARD auto GetQueue() const noexcept -> const CQueue&;

        auto SetDebugName(std::string_view name) noexcept -> void;

        auto Begin() noexcept -> Self&;

        auto BeginRendering(const SRenderingInfo& renderingInfo) noexcept -> Self&;
        auto SetViewport() noexcept -> Self&;
        auto SetScissor() noexcept -> Self&;
        auto SetViewport(const SViewport& viewport) noexcept -> Self&;
        auto SetScissor(const SScissor& scissor) noexcept -> Self&;
        auto BindPipeline(const IPipeline& pipeline) noexcept -> Self&;
        auto BindDescriptorSet(const CDescriptorSet& descriptorSet) noexcept -> Self&;
        auto PushConstants(std::span<const uint8> values, uint32 offset = 0) noexcept -> Self&;
        template <typename... Args>
        auto PushConstants(Args&&... args) noexcept -> Self&;
        auto TraceRays(uint32 width = 1, uint32 height = 1, uint32 depth = 1) noexcept -> Self&;
        auto Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance) noexcept -> Self&;
        auto EndRendering() noexcept -> Self&;

        auto MemoryBarrier(const SMemoryBarrier& memoryBarrier) noexcept -> Self&;
        auto BufferMemoryBarrier(const SBufferMemoryBarrier& memoryBarrier) noexcept -> Self&;
        auto ImageMemoryBarrier(const SImageMemoryBarrier& imageMemoryBarrier) noexcept -> Self&;

        auto CopyBuffer(const CBuffer& source, const CBuffer& dest, const SBufferCopyRegion& copyRegion) noexcept -> Self&;

        auto CopyImage(const CImage& source, const CImage& dest, const SImageCopyRegion& copyRegion) noexcept -> Self&;
        auto BlitImage(const CImage& source, const CImage& dest, const SImageBlitRegion& blitRegion) noexcept -> Self&;

        auto ResetQueryPool(const CQueryPool& queryPool, uint32 firstQuery, uint32 queryCount = -1_u32) noexcept -> Self&;
        auto BeginQuery(const CQueryPool& queryPool, uint32 query, bool isPrecise = false) noexcept -> Self&;
        auto EndQuery(const CQueryPool& queryPool, uint32 query) noexcept -> Self&;
        auto WriteAccelerationStructureProperties(
            const CQueryPool& queryPool,
            const IAccelerationStructure& accelerationStructure,
            uint32 firstQuery
        ) noexcept -> Self&;

        auto BuildAccelerationStructure(
            const SAccelerationStructureBuildInfo& buildInfo
        ) noexcept -> Self&;
        auto CopyAccelerationStructure(
            const SAccelerationStructureCopyInfo& copyInfo
        ) noexcept -> Self&;

        auto BeginNamedRegion(std::string_view regionInfo) noexcept -> Self&;
        auto EndNamedRegion() noexcept -> Self&;

        auto End() noexcept -> Self&;

    private:
        VkCommandBuffer _handle = {};

        Private::SRenderingInfoState _currentRenderingState = {};
        Private::SPipelineInfoState _currentPipelineState = {};

        SCommandBufferCreateInfo _createInfo = {};
        CArcPtr<const CCommandPool> _commandPool;
        CArcPtr<const CQueue> _queue;
    };

    template <typename... Args>
    auto CCommandBuffer::PushConstants(Args&&... args) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        PushConstants(std::span<const uint8>(MakeByteArray(std::forward<Args>(args)...)));
        return *this;
    }
}
