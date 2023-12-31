#include <Retina/Graphics/RayTracing/AccelerationStructure.hpp>
#include <Retina/Graphics/RayTracing/AccelerationStructureInfo.hpp>

#include <Retina/Graphics/Buffer.hpp>
#include <Retina/Graphics/CommandBuffer.hpp>
#include <Retina/Graphics/CommandPool.hpp>
#include <Retina/Graphics/DescriptorLayout.hpp>
#include <Retina/Graphics/DescriptorSet.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/Pipeline.hpp>
#include <Retina/Graphics/ImageView.hpp>
#include <Retina/Graphics/QueryPool.hpp>
#include <Retina/Graphics/Queue.hpp>

#include <volk.h>

namespace Retina {
    RETINA_NODISCARD static auto ToPipelineBindPoint(EPipelineType type) noexcept -> VkPipelineBindPoint {
        RETINA_PROFILE_SCOPED();
        switch (type) {
            case EPipelineType::E_COMPUTE:
                return VK_PIPELINE_BIND_POINT_COMPUTE;
            case EPipelineType::E_GRAPHICS:
                return VK_PIPELINE_BIND_POINT_GRAPHICS;
            case EPipelineType::E_RAY_TRACING:
                return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
        }
        std::unreachable();
    }

    RETINA_NODISCARD static auto MakeNativeRenderingAttachmentInfo(
        const SAttachmentInfo& attachmentInfo,
        EImageLayout layout
    ) noexcept -> VkRenderingAttachmentInfo {
        RETINA_PROFILE_SCOPED();
        const auto& image = attachmentInfo.Image.get();
        auto info = VkRenderingAttachmentInfo(VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO);
        info.imageView = image.GetView().GetHandle();
        info.imageLayout = ToEnumCounterpart(layout);
        info.resolveMode = VK_RESOLVE_MODE_NONE;
        info.resolveImageView = {};
        info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.loadOp = ToEnumCounterpart(attachmentInfo.LoadOperation);
        info.storeOp = ToEnumCounterpart(attachmentInfo.StoreOperation);
        info.clearValue = std::bit_cast<VkClearValue>(attachmentInfo.ClearValue);
        return info;
    }

    RETINA_NODISCARD static auto MakeNativeMemoryBarrier(const SMemoryBarrier& memoryBarrier) noexcept -> VkMemoryBarrier2 {
        RETINA_PROFILE_SCOPED();
        auto info = VkMemoryBarrier2(VK_STRUCTURE_TYPE_MEMORY_BARRIER_2);
        info.srcStageMask = ToEnumCounterpart(memoryBarrier.SourceStage);
        info.srcAccessMask = ToEnumCounterpart(memoryBarrier.SourceAccess);
        info.dstStageMask = ToEnumCounterpart(memoryBarrier.DestStage);
        info.dstAccessMask = ToEnumCounterpart(memoryBarrier.DestAccess);
        return info;
    }

    RETINA_NODISCARD static auto MakeNativeBufferMemoryBarrier(const SBufferMemoryBarrier& memoryBarrier) noexcept -> VkBufferMemoryBarrier2 {
        RETINA_PROFILE_SCOPED();
        const auto& buffer = memoryBarrier.Buffer.get();
        auto info = VkBufferMemoryBarrier2(VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2);
        info.srcStageMask = ToEnumCounterpart(memoryBarrier.SourceStage);
        info.srcAccessMask = ToEnumCounterpart(memoryBarrier.SourceAccess);
        info.dstStageMask = ToEnumCounterpart(memoryBarrier.DestStage);
        info.dstAccessMask = ToEnumCounterpart(memoryBarrier.DestAccess);
        info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        info.buffer = buffer.GetHandle();
        info.offset = memoryBarrier.Offset;
        info.size = memoryBarrier.Size;
        return info;
    }

    RETINA_NODISCARD static auto MakeNativeImageMemoryBarrier(const SImageMemoryBarrier& memoryBarrier) noexcept -> VkImageMemoryBarrier2 {
        RETINA_PROFILE_SCOPED();
        const auto& image = memoryBarrier.Image.get();
        auto info = VkImageMemoryBarrier2(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2);
        info.srcStageMask = ToEnumCounterpart(memoryBarrier.SourceStage);
        info.srcAccessMask = ToEnumCounterpart(memoryBarrier.SourceAccess);
        info.dstStageMask = ToEnumCounterpart(memoryBarrier.DestStage);
        info.dstAccessMask = ToEnumCounterpart(memoryBarrier.DestAccess);
        info.oldLayout = ToEnumCounterpart(memoryBarrier.OldLayout);
        info.newLayout = ToEnumCounterpart(memoryBarrier.NewLayout);
        info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        info.image = image.GetHandle();
        info.subresourceRange = MakeNativeImageSubresourceRange(image.GetView().GetAspectMask(), memoryBarrier.Subresource);
        return info;
    }

    CCommandBuffer::~CCommandBuffer() noexcept {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(_queue->GetDevice().GetLogger(), "Destroying Command Buffer: \"{}\"", GetDebugName());
        vkFreeCommandBuffers(_queue->GetDevice().GetHandle(), _commandPool->GetHandle(), 1, &_handle);
    }

    auto CCommandBuffer::Make(const CQueue& queue, const SCommandBufferCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto commandBuffer = CArcPtr(new Self());
        auto commandPoolInfo = *createInfo.CommandPoolInfo;
        if (commandPoolInfo.Name.empty()) {
            commandPoolInfo.Name = std::format("{}{}", createInfo.Name, "CommandPool");
        }
        auto commandPool = CCommandPool::Make(queue, commandPoolInfo);
        RETINA_LOG_INFO(
            queue.GetLogger(),
            "Creating Command Buffer: \"{}\" with Command Pool: \"{}\"",
            createInfo.Name,
            commandPool->GetDebugName()
        );
        auto commandBufferAllocateInfo = VkCommandBufferAllocateInfo(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
        commandBufferAllocateInfo.commandPool = commandPool->GetHandle();
        commandBufferAllocateInfo.level = createInfo.Primary
            ? VK_COMMAND_BUFFER_LEVEL_PRIMARY
            : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        auto commandBufferHandle = VkCommandBuffer();
        RETINA_VULKAN_CHECK(
            queue.GetLogger(),
            vkAllocateCommandBuffers(
                queue.GetDevice().GetHandle(),
                &commandBufferAllocateInfo,
                &commandBufferHandle
            )
        );
        commandBuffer->_handle = commandBufferHandle;
        commandBuffer->_createInfo = createInfo;
        commandBuffer->_commandPool = commandPool.AsConst();
        commandBuffer->_queue = queue.ToArcPtr();
        commandBuffer->SetDebugName(createInfo.Name);
        return commandBuffer;
    }

    auto CCommandBuffer::MakeWith(const CCommandPool& pool, const SCommandBufferCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto commandBuffer = CArcPtr(new Self());
        RETINA_LOG_INFO(
            pool.GetQueue().GetLogger(),
            "Creating Command Buffer: \"{}\" with Command Pool: \"{}\"",
            createInfo.Name,
            pool.GetDebugName()
        );
        auto commandBufferAllocateInfo = VkCommandBufferAllocateInfo(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
        commandBufferAllocateInfo.commandPool = pool.GetHandle();
        commandBufferAllocateInfo.level = createInfo.Primary
            ? VK_COMMAND_BUFFER_LEVEL_PRIMARY
            : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

        auto commandBufferHandle = VkCommandBuffer();
        RETINA_VULKAN_CHECK(
            pool.GetQueue().GetLogger(),
            vkAllocateCommandBuffers(
                pool.GetQueue().GetDevice().GetHandle(),
                &commandBufferAllocateInfo,
                &commandBufferHandle
            )
        );
        commandBuffer->_handle = commandBufferHandle;
        commandBuffer->_createInfo = createInfo;
        commandBuffer->_commandPool = pool.ToArcPtr();
        commandBuffer->_queue = pool.GetQueue().ToArcPtr();
        commandBuffer->SetDebugName(createInfo.Name);
        return commandBuffer;
    }

    auto CCommandBuffer::Make(
        const CQueue& queue,
        uint32 count,
        const SCommandBufferCreateInfo& createInfo
    ) noexcept -> std::vector<CArcPtr<Self>> {
        RETINA_PROFILE_SCOPED();
        auto commandBuffers = std::vector<CArcPtr<Self>>();
        commandBuffers.reserve(count);
        for (auto i = 0_u32; i < count; ++i) {
            auto newCreateInfo = createInfo;
            newCreateInfo.Name += std::to_string(i);
            commandBuffers.push_back(Make(queue, newCreateInfo));
        }
        return commandBuffers;
    }

    auto CCommandBuffer::MakeWith(
        const CCommandPool& pool,
        uint32 count,
        const SCommandBufferCreateInfo& createInfo
    ) noexcept -> std::vector<CArcPtr<Self>> {
        RETINA_PROFILE_SCOPED();
        auto commandBuffers = std::vector<CArcPtr<Self>>();
        commandBuffers.reserve(count);
        for (auto i = 0_u32; i < count; ++i) {
            auto newCreateInfo = createInfo;
            newCreateInfo.Name += std::to_string(i);
            commandBuffers.push_back(MakeWith(pool, newCreateInfo));
        }
        return commandBuffers;
    }

    auto CCommandBuffer::GetHandle() const noexcept -> VkCommandBuffer {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto CCommandBuffer::GetCreateInfo() const noexcept -> const SCommandBufferCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CCommandBuffer::GetCommandPool() const noexcept -> const CCommandPool& {
        RETINA_PROFILE_SCOPED();
        return *_commandPool;
    }

    auto CCommandBuffer::GetQueue() const noexcept -> const CQueue& {
        RETINA_PROFILE_SCOPED();
        return *_queue;
    }

    auto CCommandBuffer::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(
            _queue->GetDevice().GetLogger(),
            vkSetDebugUtilsObjectNameEXT(
                _queue->GetDevice().GetHandle(),
                &info
            )
        );
    }

    auto CCommandBuffer::Begin() noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        auto beginInfo = VkCommandBufferBeginInfo(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        RETINA_VULKAN_CHECK(_queue->GetLogger(), vkBeginCommandBuffer(_handle, &beginInfo));
        return *this;
    }

    auto CCommandBuffer::BeginRendering(const SRenderingInfo& renderingInfo) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        auto regionName = renderingInfo.Name;
        if (regionName.empty()) {
            regionName = "GenericRegion";
        }
        BeginNamedRegion(regionName);

        auto viewportSize = SExtent2D();
        auto layerCount = 0_u32;
        auto colorAttachments = std::vector<VkRenderingAttachmentInfo>();
        colorAttachments.reserve(renderingInfo.ColorAttachments.size());
        for (auto& attachmentInfo : renderingInfo.ColorAttachments) {
            const auto& image = attachmentInfo.Image.get();
            colorAttachments.emplace_back(
                MakeNativeRenderingAttachmentInfo(
                    attachmentInfo,
                    EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL
                )
            );
            viewportSize = { image.GetWidth(), image.GetHeight() };
            layerCount = image.GetLayers();
        }
        auto isDepthStencil = false;
        auto depthAttachment = std::optional<VkRenderingAttachmentInfo>();
        if (renderingInfo.DepthAttachment) {
            const auto& attachmentInfo = *renderingInfo.DepthAttachment;
            const auto& image = attachmentInfo.Image.get();
            const auto aspectMask = image.GetView().GetAspectMask();
            const auto targetAspectMask = EImageAspect::E_DEPTH | EImageAspect::E_STENCIL;
            if (IsFlagEnabled(aspectMask, targetAspectMask)) {
                isDepthStencil = true;
            }
            depthAttachment = MakeNativeRenderingAttachmentInfo(
                attachmentInfo,
                isDepthStencil
                    ? EImageLayout::E_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                    : EImageLayout::E_DEPTH_ATTACHMENT_OPTIMAL
            );
            viewportSize = { image.GetWidth(), image.GetHeight() };
            layerCount = image.GetLayers();
        }
        auto stencilAttachment = std::optional<VkRenderingAttachmentInfo>();
        if (isDepthStencil) {
            stencilAttachment = *depthAttachment;
        } else if (renderingInfo.StencilAttachment) {
            const auto& attachmentInfo = *renderingInfo.StencilAttachment;
            const auto& image = attachmentInfo.Image.get();
            stencilAttachment = MakeNativeRenderingAttachmentInfo(
                attachmentInfo,
                EImageLayout::E_STENCIL_ATTACHMENT_OPTIMAL
            );
            viewportSize = { image.GetWidth(), image.GetHeight() };
            layerCount = image.GetLayers();
        }

        auto info = VkRenderingInfo(VK_STRUCTURE_TYPE_RENDERING_INFO);
        info.renderArea = std::bit_cast<VkRect2D>(renderingInfo.RenderArea);
        if (renderingInfo.RenderArea == SRect2D()) {
            info.renderArea = {
                .offset = {},
                .extent = {
                    .width = viewportSize.Width,
                    .height = viewportSize.Height
                }
            };
        }
        info.layerCount = renderingInfo.LayerCount;
        if (renderingInfo.LayerCount == -1_u32) {
            info.layerCount = layerCount;
        }
        info.viewMask = renderingInfo.ViewMask;
        info.colorAttachmentCount = colorAttachments.size();
        info.pColorAttachments = colorAttachments.data();
        info.pDepthAttachment = depthAttachment
            ? &depthAttachment.value()
            : nullptr;
        info.pStencilAttachment = stencilAttachment
            ? &stencilAttachment.value()
            : nullptr;
        vkCmdBeginRendering(_handle, &info);

        _currentRenderingState.RenderArea = std::bit_cast<SRect2D>(info.renderArea);
        return *this;
    }

    auto CCommandBuffer::SetViewport() noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        SetViewport({
            .X = 0.0f,
            .Y = 0.0f,
            .Width = static_cast<float32>(_currentRenderingState.RenderArea.Extent.Width),
            .Height = static_cast<float32>(_currentRenderingState.RenderArea.Extent.Height),
        });
        return *this;
    }

    auto CCommandBuffer::SetScissor() noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        SetScissor({
            .X = 0,
            .Y = 0,
            .Width = _currentRenderingState.RenderArea.Extent.Width,
            .Height = _currentRenderingState.RenderArea.Extent.Height,
        });
        return *this;
    }

    auto CCommandBuffer::SetViewport(const SViewport& viewport) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        auto viewportInfo = VkViewport();
        viewportInfo.x = viewport.X;
        viewportInfo.y = viewport.Y;
        viewportInfo.width = viewport.Width;
        viewportInfo.height = viewport.Height;
        viewportInfo.minDepth = 0.0f;
        viewportInfo.maxDepth = 1.0f;
        vkCmdSetViewport(_handle, 0, 1, &viewportInfo);
        return *this;
    }

    auto CCommandBuffer::SetScissor(const SScissor& scissor) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        auto scissorInfo = std::bit_cast<VkRect2D>(scissor);
        vkCmdSetScissor(_handle, 0, 1, &scissorInfo);
        return *this;
    }

    auto CCommandBuffer::BindPipeline(const IPipeline& pipeline) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        vkCmdBindPipeline(_handle, ToPipelineBindPoint(pipeline.GetType()), pipeline.GetHandle());
        _currentPipelineState.Pipeline = &pipeline;
        return *this;
    }

    auto CCommandBuffer::BindDescriptorSet(const CDescriptorSet& descriptorSet) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        const auto type = ToPipelineBindPoint(_currentPipelineState.Pipeline->GetType());
        const auto layoutHandle = _currentPipelineState.Pipeline->GetLayoutHandle();
        vkCmdBindDescriptorSets(_handle, type, layoutHandle, 0, 1, AsConstPtr(descriptorSet.GetHandle()), 0, nullptr);
        return *this;
    }

    auto CCommandBuffer::PushConstants(std::span<const uint8> values, uint32 offset) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        const auto& [handle, pushConstantInfo] = _currentPipelineState.Pipeline->GetLayout();
        vkCmdPushConstants(
            _handle,
            handle,
            ToEnumCounterpart(pushConstantInfo.ShaderStage),
            offset,
            values.size(),
            values.data()
        );
        return *this;
    }

    auto CCommandBuffer::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        vkCmdDraw(_handle, vertexCount, instanceCount, firstVertex, firstInstance);
        return *this;
    }

    auto CCommandBuffer::EndRendering() noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        vkCmdEndRendering(_handle);
        EndNamedRegion();
        return *this;
    }

    auto CCommandBuffer::MemoryBarrier(const SMemoryBarrier& memoryBarrier) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        const auto barrier = MakeNativeMemoryBarrier(memoryBarrier);
        auto dependencyInfo = VkDependencyInfo(VK_STRUCTURE_TYPE_DEPENDENCY_INFO);
        dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencyInfo.memoryBarrierCount = 1;
        dependencyInfo.pMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(_handle, &dependencyInfo);
        return *this;
    }

    auto CCommandBuffer::BufferMemoryBarrier(const SBufferMemoryBarrier& memoryBarrier) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        const auto barrier = MakeNativeBufferMemoryBarrier(memoryBarrier);
        auto dependencyInfo = VkDependencyInfo(VK_STRUCTURE_TYPE_DEPENDENCY_INFO);
        dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencyInfo.bufferMemoryBarrierCount = 1;
        dependencyInfo.pBufferMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(_handle, &dependencyInfo);
        return *this;
    }

    auto CCommandBuffer::ImageMemoryBarrier(const SImageMemoryBarrier& imageMemoryBarrier) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        const auto barrier = MakeNativeImageMemoryBarrier(imageMemoryBarrier);
        auto dependencyInfo = VkDependencyInfo(VK_STRUCTURE_TYPE_DEPENDENCY_INFO);
        dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(_handle, &dependencyInfo);
        return *this;
    }

    auto CCommandBuffer::CopyBuffer(
        const CBuffer& source,
        const CBuffer& dest,
        const SBufferCopyRegion& copyRegion
    ) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        auto region = VkBufferCopy2(VK_STRUCTURE_TYPE_BUFFER_COPY_2);
        region.srcOffset = copyRegion.SourceOffset;
        region.dstOffset = copyRegion.DestOffset;
        region.size = copyRegion.Size == Constant::WHOLE_SIZE
            ? source.GetSize()
            : copyRegion.Size;

        auto copy = VkCopyBufferInfo2(VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2);
        copy.srcBuffer = source.GetHandle();
        copy.dstBuffer = dest.GetHandle();
        copy.regionCount = 1;
        copy.pRegions = &region;
        vkCmdCopyBuffer2(_handle, &copy);
        return *this;
    }

    auto CCommandBuffer::CopyImage(
        const CImage& source,
        const CImage& dest,
        const SImageCopyRegion& copyRegion
    ) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        const auto sourceSubresource = MakeNativeImageSubresourceLayers(source, copyRegion.SourceSubresource);
        const auto destSubresource = MakeNativeImageSubresourceLayers(dest, copyRegion.DestSubresource);
        const auto sourceOffset = std::bit_cast<VkOffset3D>(copyRegion.SourceOffset);
        const auto destOffset = std::bit_cast<VkOffset3D>(copyRegion.DestOffset);
        auto region = VkImageCopy2(VK_STRUCTURE_TYPE_IMAGE_COPY_2);
        region.srcSubresource = sourceSubresource;
        region.srcOffset = sourceOffset;
        region.dstSubresource = destSubresource;
        region.dstOffset = destOffset;
        region.extent = std::bit_cast<VkExtent3D>(copyRegion.Extent);

        auto copy = VkCopyImageInfo2(VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2);
        copy.srcImage = source.GetHandle();
        copy.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        copy.dstImage = dest.GetHandle();
        copy.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copy.regionCount = 1;
        copy.pRegions = &region;
        vkCmdCopyImage2(_handle, &copy);
        return *this;
    }

    auto CCommandBuffer::BlitImage(
        const CImage& source,
        const CImage& dest,
        const SImageBlitRegion& blitRegion
    ) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        const auto sourceSubresource = MakeNativeImageSubresourceLayers(source, blitRegion.SourceSubresource);
        const auto destSubresource = MakeNativeImageSubresourceLayers(dest, blitRegion.DestSubresource);
        const auto sourceOffsets = std::bit_cast<std::array<VkOffset3D, 2>>(blitRegion.SourceOffsets);
        const auto destOffsets = std::bit_cast<std::array<VkOffset3D, 2>>(blitRegion.DestOffsets);
        auto region = VkImageBlit2(VK_STRUCTURE_TYPE_IMAGE_BLIT_2);
        region.srcSubresource = sourceSubresource;
        region.srcOffsets[0] = sourceOffsets[0];
        if (blitRegion.SourceOffsets[1] == SOffset3D()) {
            region.srcOffsets[1] = {
                .x = static_cast<int32>(source.GetWidth()),
                .y = static_cast<int32>(source.GetHeight()),
                .z = 1
            };
        } else {
            region.srcOffsets[1] = sourceOffsets[1];
        }
        region.dstSubresource = destSubresource;
        region.dstOffsets[0] = destOffsets[0];
        if (blitRegion.DestOffsets[1] == SOffset3D()) {
            region.dstOffsets[1] = {
                .x = static_cast<int32>(dest.GetWidth()),
                .y = static_cast<int32>(dest.GetHeight()),
                .z = 1
            };
        } else {
            region.dstOffsets[1] = destOffsets[1];
        }

        auto blit = VkBlitImageInfo2(VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2);
        blit.srcImage = source.GetHandle();
        blit.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blit.dstImage = dest.GetHandle();
        blit.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blit.regionCount = 1;
        blit.pRegions = &region;
        vkCmdBlitImage2(_handle, &blit);
        return *this;
    }

    auto CCommandBuffer::ResetQueryPool(const CQueryPool& queryPool, uint32 firstQuery, uint32 queryCount) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        if (queryCount == -1_u32) {
            queryCount = queryPool.GetCount() - firstQuery;
        }
        vkCmdResetQueryPool(_handle, queryPool.GetHandle(), firstQuery, queryCount);
        return *this;
    }

    auto CCommandBuffer::BeginQuery(const CQueryPool& queryPool, uint32 query, bool isPrecise) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        vkCmdBeginQuery(_handle, queryPool.GetHandle(), query, isPrecise ? VK_QUERY_CONTROL_PRECISE_BIT : 0);
        return *this;
    }

    auto CCommandBuffer::EndQuery(const CQueryPool& queryPool, uint32 query) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        vkCmdEndQuery(_handle, queryPool.GetHandle(), query);
        return *this;
    }

    auto CCommandBuffer::WriteAccelerationStructureProperties(
        const CQueryPool& queryPool,
        const IAccelerationStructure& accelerationStructure,
        uint32 firstQuery
    ) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        vkCmdWriteAccelerationStructuresPropertiesKHR(
            _handle,
            1,
            AsConstPtr(accelerationStructure.GetHandle()),
            ToEnumCounterpart(queryPool.GetType()),
            queryPool.GetHandle(),
            firstQuery
        );
        return *this;
    }

    auto CCommandBuffer::BuildAccelerationStructure(
        const SAccelerationStructureBuildInfo& buildInfo
    ) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        auto buildGeometryInfos = MakeNativeAccelerationStructureGeometryInfo(buildInfo.GeometryInfos);
        auto buildInfosNative = VkAccelerationStructureBuildGeometryInfoKHR(VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR);
        buildInfosNative.type = ToEnumCounterpart(buildInfo.Type);
        buildInfosNative.flags = ToEnumCounterpart(buildInfo.Flags);
        buildInfosNative.mode = ToEnumCounterpart(buildInfo.Mode);
        buildInfosNative.srcAccelerationStructure = buildInfo.Source;
        buildInfosNative.dstAccelerationStructure = buildInfo.Dest;
        buildInfosNative.geometryCount = buildGeometryInfos.size();
        buildInfosNative.pGeometries = buildGeometryInfos.data();
        buildInfosNative.scratchData.deviceAddress = buildInfo.ScratchBuffer->GetAddress();

        auto buildRangeInfosNative = std::vector<VkAccelerationStructureBuildRangeInfoKHR>();
        buildRangeInfosNative.reserve(buildInfo.GeometryInfos.size());
        for (const auto& each : buildInfo.GeometryInfos) {
            buildRangeInfosNative.emplace_back(std::bit_cast<VkAccelerationStructureBuildRangeInfoKHR>(each.Range));
        }

        vkCmdBuildAccelerationStructuresKHR(
            _handle,
            1,
            &buildInfosNative,
            AsConstPtr(buildRangeInfosNative.data())
        );
        return *this;
    }

    auto CCommandBuffer::CopyAccelerationStructure(
        const SAccelerationStructureCopyInfo& copyInfo
    ) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        auto copyInfosNative = VkCopyAccelerationStructureInfoKHR(VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR);
        copyInfosNative.src = copyInfo.Source;
        copyInfosNative.dst = copyInfo.Dest;
        copyInfosNative.mode = ToEnumCounterpart(copyInfo.Mode);
        vkCmdCopyAccelerationStructureKHR(_handle, &copyInfosNative);
        return *this;
    }

    auto CCommandBuffer::BeginNamedRegion(std::string_view regionInfo) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        auto region = VkDebugUtilsLabelEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT);
        region.pLabelName = regionInfo.data();
        vkCmdBeginDebugUtilsLabelEXT(_handle, &region);
        return *this;
    }

    auto CCommandBuffer::EndNamedRegion() noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        vkCmdEndDebugUtilsLabelEXT(_handle);
        return *this;
    }

    auto CCommandBuffer::End() noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        RETINA_VULKAN_CHECK(_queue->GetLogger(), vkEndCommandBuffer(_handle));
        return *this;
    }
}
