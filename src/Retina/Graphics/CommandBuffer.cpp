#include <Retina/Graphics/Resources/ShaderResourceTable.hpp>
#include <Retina/Graphics/Buffer.hpp>
#include <Retina/Graphics/CommandBuffer.hpp>
#include <Retina/Graphics/CommandPool.hpp>
#include <Retina/Graphics/DescriptorSet.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/ImageView.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>
#include <Retina/Graphics/Pipeline.hpp>
#include <Retina/Graphics/Queue.hpp>

#include <volk.h>

namespace Retina::Graphics {
  namespace Details {
    RETINA_NODISCARD RETINA_INLINE auto MakeNativeRenderingAttachmentInfo(
      const SAttachmentInfo& attachmentInfo,
      EImageLayout layout
    ) noexcept -> VkRenderingAttachmentInfo {
      RETINA_PROFILE_SCOPED();
      const auto& imageView = *attachmentInfo.ImageView;
      auto info = VkRenderingAttachmentInfo(VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO);
      info.imageView = imageView.GetHandle();
      info.imageLayout = AsEnumCounterpart(layout);
      info.resolveMode = VK_RESOLVE_MODE_NONE;
      info.resolveImageView = {};
      info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      info.loadOp = AsEnumCounterpart(attachmentInfo.LoadOperator);
      info.storeOp = AsEnumCounterpart(attachmentInfo.StoreOperator);
      info.clearValue = std::bit_cast<VkClearValue>(attachmentInfo.ClearValue);
      return info;
    }

    RETINA_NODISCARD RETINA_INLINE auto MakeNativeMemoryBarrier(const SMemoryBarrier& barrier) noexcept -> VkMemoryBarrier2 {
      RETINA_PROFILE_SCOPED();
      auto info = VkMemoryBarrier2(VK_STRUCTURE_TYPE_MEMORY_BARRIER_2);
      info.srcStageMask = AsEnumCounterpart(barrier.SourceStage);
      info.srcAccessMask = AsEnumCounterpart(barrier.SourceAccess);
      info.dstStageMask = AsEnumCounterpart(barrier.DestStage);
      info.dstAccessMask = AsEnumCounterpart(barrier.DestAccess);
      return info;
    }

    RETINA_NODISCARD RETINA_INLINE auto MakeNativeBufferMemoryBarrier(const SBufferMemoryBarrier& barrier) noexcept -> VkBufferMemoryBarrier2 {
      RETINA_PROFILE_SCOPED();
      const auto& buffer = *barrier.Buffer;
      auto info = VkBufferMemoryBarrier2(VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2);
      info.srcStageMask = AsEnumCounterpart(barrier.SourceStage);
      info.srcAccessMask = AsEnumCounterpart(barrier.SourceAccess);
      info.dstStageMask = AsEnumCounterpart(barrier.DestStage);
      info.dstAccessMask = AsEnumCounterpart(barrier.DestAccess);
      // TODO: Handle queue family ownership transfers
      info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      info.buffer = buffer.GetHandle();
      info.offset = barrier.MemoryRange.Offset;
      info.size = barrier.MemoryRange.Size;
      return info;
    }

    RETINA_NODISCARD RETINA_INLINE auto MakeNativeImageMemoryBarrier(const SImageMemoryBarrier& barrier) noexcept -> VkImageMemoryBarrier2 {
      RETINA_PROFILE_SCOPED();
      const auto& image = *barrier.Image;
      auto info = VkImageMemoryBarrier2(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2);
      info.srcStageMask = AsEnumCounterpart(barrier.SourceStage);
      info.srcAccessMask = AsEnumCounterpart(barrier.SourceAccess);
      info.dstStageMask = AsEnumCounterpart(barrier.DestStage);
      info.dstAccessMask = AsEnumCounterpart(barrier.DestAccess);
      info.oldLayout = AsEnumCounterpart(barrier.OldLayout);
      info.newLayout = AsEnumCounterpart(barrier.NewLayout);
      // TODO: Handle queue family ownership transfers
      info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      info.image = image.GetHandle();
      info.subresourceRange = MakeNativeImageSubresourceRange(image.GetView().GetAspectMask(), barrier.SubresourceRange);
      return info;
    }

    RETINA_INLINE auto IssueFullBarrier(VkCommandBuffer commandBuffer) noexcept -> void {
      RETINA_PROFILE_SCOPED();
      auto barrier = VkMemoryBarrier2(VK_STRUCTURE_TYPE_MEMORY_BARRIER_2);
      barrier.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
      barrier.dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;

      auto dependencyInfo = VkDependencyInfo(VK_STRUCTURE_TYPE_DEPENDENCY_INFO);
      dependencyInfo.memoryBarrierCount = 1;
      dependencyInfo.pMemoryBarriers = &barrier;
      vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
    }
  }

  CCommandBuffer::~CCommandBuffer() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      vkFreeCommandBuffers(GetQueue().GetDevice().GetHandle(), GetCommandPool().GetHandle(), 1, &_handle);
      RETINA_GRAPHICS_INFO("Command buffer ({}) destroyed", GetDebugName());
    }
  }

  auto CCommandBuffer::Make(
    const CQueue& queue,
    const SCommandBufferCreateInfo& createInfo
  ) noexcept -> Core::CArcPtr<CCommandBuffer> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CCommandBuffer());
    auto commandPoolCreateInfo = *createInfo.PoolInfo;
    if (commandPoolCreateInfo.Name.empty()) {
      commandPoolCreateInfo.Name = std::format("{}Pool", createInfo.Name);
    }
    const auto commandPool = CCommandPool::Make(queue, commandPoolCreateInfo);
    const auto commandPoolHandle = commandPool->GetHandle();

    auto commandBufferAllocateInfo = VkCommandBufferAllocateInfo(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
    commandBufferAllocateInfo.commandPool = commandPoolHandle;
    commandBufferAllocateInfo.level = AsEnumCounterpart(createInfo.Level);
    commandBufferAllocateInfo.commandBufferCount = 1;

    auto commandBufferHandle = VkCommandBuffer();
    RETINA_GRAPHICS_VULKAN_CHECK(
      vkAllocateCommandBuffers(
        queue.GetDevice().GetHandle(),
        &commandBufferAllocateInfo,
        &commandBufferHandle
      )
    );
    RETINA_GRAPHICS_INFO("Command buffer ({}) initialized", createInfo.Name);

    self->_handle = commandBufferHandle;
    self->_createInfo = createInfo;
    self->_commandPool = commandPool;
    self->_queue = queue.ToArcPtr();
    self->SetDebugName(createInfo.Name);
    return self;
  }

  auto CCommandBuffer::Make(
    const CQueue& queue,
    uint32 count,
    const SCommandBufferCreateInfo& createInfo
  ) noexcept -> std::vector<Core::CArcPtr<CCommandBuffer>> {
    RETINA_PROFILE_SCOPED();
    auto commandBuffers = std::vector<Core::CArcPtr<CCommandBuffer>>();
    commandBuffers.reserve(count);
    for (auto i = 0_u32; i < count; ++i) {
      auto currentCreateInfo = createInfo;
      currentCreateInfo.Name += std::to_string(i);
      commandBuffers.emplace_back(Make(queue, currentCreateInfo));
    }
    return commandBuffers;
  }

  auto CCommandBuffer::MakeWith(
    CCommandPool& commandPool,
    const SCommandBufferCreateInfo& createInfo
  ) noexcept -> Core::CArcPtr<CCommandBuffer> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CCommandBuffer());
    const auto commandPoolHandle = commandPool.GetHandle();
    auto commandBufferAllocateInfo = VkCommandBufferAllocateInfo(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
    commandBufferAllocateInfo.commandPool = commandPoolHandle;
    commandBufferAllocateInfo.level = AsEnumCounterpart(createInfo.Level);
    commandBufferAllocateInfo.commandBufferCount = 1;

    auto commandBufferHandle = VkCommandBuffer();
    RETINA_GRAPHICS_VULKAN_CHECK(
      vkAllocateCommandBuffers(
        commandPool.GetQueue().GetDevice().GetHandle(),
        &commandBufferAllocateInfo,
        &commandBufferHandle
      )
    );
    RETINA_GRAPHICS_INFO("Command buffer ({}) initialized", createInfo.Name);

    self->_handle = commandBufferHandle;
    self->_createInfo = createInfo;
    self->_commandPool = commandPool.ToArcPtr();
    self->_queue = commandPool.GetQueue().ToArcPtr();
    self->SetDebugName(createInfo.Name);

    return self;
  }

  auto CCommandBuffer::MakeWith(
    CCommandPool& commandPool,
    uint32 count,
    const SCommandBufferCreateInfo& createInfo
  ) noexcept -> std::vector<Core::CArcPtr<CCommandBuffer>> {
    RETINA_PROFILE_SCOPED();
    auto commandBuffers = std::vector<Core::CArcPtr<CCommandBuffer>>();
    commandBuffers.reserve(count);
    for (auto i = 0_u32; i < count; ++i) {
      auto currentCreateInfo = createInfo;
      currentCreateInfo.Name += std::to_string(i);
      commandBuffers.emplace_back(MakeWith(commandPool, currentCreateInfo));
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

  auto CCommandBuffer::GetCommandPool() const noexcept -> CCommandPool& {
    RETINA_PROFILE_SCOPED();
    return *_commandPool;
  }

  auto CCommandBuffer::GetQueue() const noexcept -> const CQueue& {
    RETINA_PROFILE_SCOPED();
    return *_queue;
  }

  auto CCommandBuffer::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CCommandBuffer::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_SET_DEBUG_NAME(GetQueue().GetDevice().GetHandle(), _handle, VK_OBJECT_TYPE_COMMAND_BUFFER, name);
    _createInfo.Name = name;
  }

  auto CCommandBuffer::Begin() noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    auto commandBufferBeginInfo = VkCommandBufferBeginInfo(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
    RETINA_GRAPHICS_VULKAN_CHECK(vkBeginCommandBuffer(_handle, &commandBufferBeginInfo));
    return *this;
  }

  auto CCommandBuffer::End() noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_VULKAN_CHECK(vkEndCommandBuffer(_handle));
    return *this;
  }

  auto CCommandBuffer::BeginRendering(SRenderingInfo renderingInfo) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    auto regionName = renderingInfo.Name;
    if (regionName.empty()) {
      regionName = "GenericRenderingRegion";
    }
    BeginNamedRegion(regionName);

    auto attachmentSize = SExtent2D();
    auto layerCount = 0_u32;

    auto colorAttachments = std::vector<VkRenderingAttachmentInfo>();
    colorAttachments.reserve(renderingInfo.ColorAttachments.size());
    for (const auto& attachment : renderingInfo.ColorAttachments) {
      const auto& imageView = *attachment.ImageView;
      const auto& image = imageView.GetImage();
      const auto subresource = imageView.GetSubresourceRange();
      colorAttachments.emplace_back(
        Details::MakeNativeRenderingAttachmentInfo(
          attachment, EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL
        )
      );
      attachmentSize = { image.GetWidth(), image.GetHeight() };
      layerCount = subresource.LayerCount;
    }

    auto depthAttachment = VkRenderingAttachmentInfo();
    if (renderingInfo.DepthAttachment) {
      const auto& attachmentInfo = *renderingInfo.DepthAttachment;
      const auto& imageView = *attachmentInfo.ImageView;
      const auto& image = imageView.GetImage();
      const auto subresource = imageView.GetSubresourceRange();
      const auto isDepthStencil = Core::IsFlagEnabled(imageView.GetAspectMask(), EImageAspectFlag::E_STENCIL);
      const auto imageLayout = isDepthStencil
        ? EImageLayout::E_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        : EImageLayout::E_DEPTH_ATTACHMENT_OPTIMAL;
      depthAttachment = Details::MakeNativeRenderingAttachmentInfo(attachmentInfo, imageLayout);
      attachmentSize = { image.GetWidth(), image.GetHeight() };
      layerCount = subresource.LayerCount;
    }

    auto stencilAttachment = VkRenderingAttachmentInfo();
    if (renderingInfo.StencilAttachment) {
      const auto& attachmentInfo = *renderingInfo.StencilAttachment;
      const auto& imageView = *attachmentInfo.ImageView;
      const auto& image = imageView.GetImage();
      const auto subresource = imageView.GetSubresourceRange();
      const auto isDepthStencil = Core::IsFlagEnabled(imageView.GetAspectMask(), EImageAspectFlag::E_STENCIL);
      const auto imageLayout = isDepthStencil
        ? EImageLayout::E_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        : EImageLayout::E_STENCIL_ATTACHMENT_OPTIMAL;
      depthAttachment = Details::MakeNativeRenderingAttachmentInfo(attachmentInfo, imageLayout);
      attachmentSize = { image.GetWidth(), image.GetHeight() };
      layerCount = subresource.LayerCount;
    }

    if (renderingInfo.RenderArea == SRect2D()) {
      renderingInfo.RenderArea = {
        .Offset = { 0, 0 },
        .Extent = attachmentSize
      };
    }
    if (renderingInfo.LayerCount == -1_u32) {
      renderingInfo.LayerCount = layerCount;
    }

    auto commandRenderingInfo = VkRenderingInfo(VK_STRUCTURE_TYPE_RENDERING_INFO);
    commandRenderingInfo.renderArea = std::bit_cast<VkRect2D>(renderingInfo.RenderArea);
    commandRenderingInfo.layerCount = renderingInfo.LayerCount;
    commandRenderingInfo.viewMask = renderingInfo.ViewMask;
    commandRenderingInfo.colorAttachmentCount = colorAttachments.size();
    commandRenderingInfo.pColorAttachments = colorAttachments.data();
    if (renderingInfo.DepthAttachment) {
      commandRenderingInfo.pDepthAttachment = &depthAttachment;
    }
    if (renderingInfo.StencilAttachment) {
      commandRenderingInfo.pStencilAttachment = &stencilAttachment;
    }

    vkCmdBeginRendering(_handle, &commandRenderingInfo);
    _currentState.RenderingInfo = std::move(renderingInfo);
    return *this;
  }

  auto CCommandBuffer::SetViewport() noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    RETINA_ASSERT_WITH(
      _currentState.RenderingInfo,
      "Cannot retrieve rendering state, did you forget to call BeginRendering()?"
    );
    const auto& renderingInfo = *_currentState.RenderingInfo;
    const auto renderArea = renderingInfo.RenderArea;
    const auto viewport = SViewport(
      static_cast<float32>(renderArea.Offset.X),
      static_cast<float32>(renderArea.Offset.Y),
      static_cast<float32>(renderArea.Extent.Width),
      static_cast<float32>(renderArea.Extent.Height)
    );
    return SetViewport(viewport);
  }

  auto CCommandBuffer::SetScissor() noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    RETINA_ASSERT_WITH(
      _currentState.RenderingInfo,
      "Cannot retrieve rendering state, did you forget to call BeginRendering()?"
    );
    const auto& renderingInfo = *_currentState.RenderingInfo;
    const auto renderArea = renderingInfo.RenderArea;
    const auto scissor = SScissor(
      renderArea.Offset.X,
      renderArea.Offset.Y,
      renderArea.Extent.Width,
      renderArea.Extent.Height
    );
    return SetScissor(scissor);
  }

  auto CCommandBuffer::SetViewport(const SViewport& viewport) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    const auto nativeViewport = VkViewport(
      viewport.X,
      viewport.Y,
      viewport.Width,
      viewport.Height,
      0.0f,
      1.0f
    );
    vkCmdSetViewport(_handle, 0, 1, &nativeViewport);
    return *this;
  }

  auto CCommandBuffer::SetScissor(const SScissor& scissor) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    const auto nativeScissor = std::bit_cast<VkRect2D>(scissor);
    vkCmdSetScissor(_handle, 0, 1, &nativeScissor);
    return *this;
  }

  auto CCommandBuffer::EndRendering() noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    vkCmdEndRendering(_handle);
    EndNamedRegion();
    _currentState.RenderingInfo = std::nullopt;
    return *this;
  }

  auto CCommandBuffer::BindPipeline(const IPipeline& pipeline) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    vkCmdBindPipeline(_handle, AsEnumCounterpart(pipeline.GetBindPoint()), pipeline.GetHandle());
    _currentState.Pipeline = &pipeline;
    return *this;
  }

  auto CCommandBuffer::BindDescriptorSet(const CDescriptorSet& descriptorSet, uint32 firstSet) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    const auto& currentPipeline = *_currentState.Pipeline;
    const auto& layout = currentPipeline.GetLayout();
    const auto bindPoint = AsEnumCounterpart(currentPipeline.GetBindPoint());
    const auto setHandle = descriptorSet.GetHandle();
    vkCmdBindDescriptorSets(_handle, bindPoint, layout.Handle, firstSet, 1, &setHandle, 0, nullptr);
    return *this;
  }

  auto CCommandBuffer::BindShaderResourceTable(const CShaderResourceTable& shaderResourceTable) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    return BindDescriptorSet(shaderResourceTable.GetDescriptorSet());
  }

  auto CCommandBuffer::PushConstants(uint32 offset, std::span<const uint8> values) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    const auto& currentPipeline = *_currentState.Pipeline;
    const auto& layout = currentPipeline.GetLayout();
    vkCmdPushConstants(_handle, layout.Handle, AsEnumCounterpart(layout.PushConstant.Stages), offset, values.size_bytes(), values.data());
    return *this;
  }

  auto CCommandBuffer::BindIndexBuffer(const CBuffer& buffer, EIndexType indexType) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    vkCmdBindIndexBuffer(_handle, buffer.GetHandle(), 0, AsEnumCounterpart(indexType));
    return *this;
  }

  auto CCommandBuffer::DrawIndexed(
    uint32 indexCount,
    uint32 instanceCount,
    uint32 firstIndex,
    int32 vertexOffset,
    uint32 firstInstance
  ) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    vkCmdDrawIndexed(_handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    return *this;
  }

  auto CCommandBuffer::Draw(
    uint32 vertexCount,
    uint32 instanceCount,
    uint32 firstVertex,
    uint32 firstInstance
  ) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    vkCmdDraw(_handle, vertexCount, instanceCount, firstVertex, firstInstance);
    return *this;
  }

  auto CCommandBuffer::DrawMeshTasks(uint32 x, uint32 y, uint32 z) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    vkCmdDrawMeshTasksEXT(_handle, x, y, z);
    return *this;
  }

  auto CCommandBuffer::Dispatch(uint32 x, uint32 y, uint32 z) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    vkCmdDispatch(_handle, x, y, z);
    return *this;
  }

  auto CCommandBuffer::Barrier(const SMemoryBarrierInfo& barrierInfo) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    auto memoryBarriers = std::vector<VkMemoryBarrier2>();
    memoryBarriers.reserve(barrierInfo.MemoryBarriers.size());
    for (const auto& barrier : barrierInfo.MemoryBarriers) {
      memoryBarriers.emplace_back(Details::MakeNativeMemoryBarrier(barrier));
    }

    auto bufferMemoryBarriers = std::vector<VkBufferMemoryBarrier2>();
    bufferMemoryBarriers.reserve(barrierInfo.BufferMemoryBarriers.size());
    for (const auto& barrier : barrierInfo.BufferMemoryBarriers) {
      bufferMemoryBarriers.emplace_back(Details::MakeNativeBufferMemoryBarrier(barrier));
    }

    auto imageMemoryBarriers = std::vector<VkImageMemoryBarrier2>();
    imageMemoryBarriers.reserve(barrierInfo.ImageMemoryBarriers.size());
    for (const auto& barrier : barrierInfo.ImageMemoryBarriers) {
      imageMemoryBarriers.emplace_back(Details::MakeNativeImageMemoryBarrier(barrier));
    }

    auto dependencyInfo = VkDependencyInfo(VK_STRUCTURE_TYPE_DEPENDENCY_INFO);
    dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencyInfo.memoryBarrierCount = memoryBarriers.size();
    dependencyInfo.pMemoryBarriers = memoryBarriers.data();
    dependencyInfo.bufferMemoryBarrierCount = bufferMemoryBarriers.size();
    dependencyInfo.pBufferMemoryBarriers = bufferMemoryBarriers.data();
    dependencyInfo.imageMemoryBarrierCount = imageMemoryBarriers.size();
    dependencyInfo.pImageMemoryBarriers = imageMemoryBarriers.data();
    vkCmdPipelineBarrier2(_handle, &dependencyInfo);
    return *this;
  }

  auto CCommandBuffer::MemoryBarrier(const SMemoryBarrier& barrier) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    auto memoryBarrier = Details::MakeNativeMemoryBarrier(barrier);
    auto dependencyInfo = VkDependencyInfo(VK_STRUCTURE_TYPE_DEPENDENCY_INFO);
    dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencyInfo.memoryBarrierCount = 1;
    dependencyInfo.pMemoryBarriers = &memoryBarrier;
    vkCmdPipelineBarrier2(_handle, &dependencyInfo);
    return *this;
  }

  auto CCommandBuffer::BufferMemoryBarrier(const SBufferMemoryBarrier& barrier) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    auto bufferBarrier = Details::MakeNativeBufferMemoryBarrier(barrier);
    auto dependencyInfo = VkDependencyInfo(VK_STRUCTURE_TYPE_DEPENDENCY_INFO);
    dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencyInfo.bufferMemoryBarrierCount = 1;
    dependencyInfo.pBufferMemoryBarriers = &bufferBarrier;
    vkCmdPipelineBarrier2(_handle, &dependencyInfo);
    return *this;
  }

  auto CCommandBuffer::ImageMemoryBarrier(const SImageMemoryBarrier& barrier) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    auto imageBarrier = Details::MakeNativeImageMemoryBarrier(barrier);
    auto dependencyInfo = VkDependencyInfo(VK_STRUCTURE_TYPE_DEPENDENCY_INFO);
    dependencyInfo.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &imageBarrier;
    vkCmdPipelineBarrier2(_handle, &dependencyInfo);
    return *this;
  }

  auto CCommandBuffer::ClearBuffer(const CBuffer& buffer, uint32 value, const SBufferMemoryRange& range) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    vkCmdFillBuffer(_handle, buffer.GetHandle(), range.Offset, range.Size, value);
    return *this;
  }

  auto CCommandBuffer::CopyBuffer(const CBuffer& source, const CBuffer& dest, const SBufferCopyRegion& copyRegion) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    auto region = VkBufferCopy2(VK_STRUCTURE_TYPE_BUFFER_COPY_2);
    region.srcOffset = copyRegion.SourceOffset;
    region.dstOffset = copyRegion.DestOffset;
    region.size = copyRegion.Size;
    if (region.size == WHOLE_SIZE) {
      region.size = source.GetSizeBytes();
    }

    auto copy = VkCopyBufferInfo2(VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2);
    copy.srcBuffer = source.GetHandle();
    copy.dstBuffer = dest.GetHandle();
    copy.regionCount = 1;
    copy.pRegions = &region;
    vkCmdCopyBuffer2(_handle, &copy);
    return *this;
  }

  auto CCommandBuffer::CopyBufferToImage(const CBuffer& source, const CImage& dest, const SBufferImageCopyRegion& copyRegion) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    const auto subresourceLayers = MakeNativeImageSubresourceLayers(dest, copyRegion.SubresourceRange);
    auto region = VkBufferImageCopy2(VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2);
    region.bufferOffset = copyRegion.Offset;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource = subresourceLayers;
    region.imageOffset = std::bit_cast<VkOffset3D>(copyRegion.ImageOffset);
    region.imageExtent = std::bit_cast<VkExtent3D>(copyRegion.ImageExtent);
    if (copyRegion.ImageExtent == SExtent3D()) {
      region.imageExtent = {
        .width = std::max(dest.GetWidth() >> subresourceLayers.mipLevel, 1_u32),
        .height = std::max(dest.GetHeight() >> subresourceLayers.mipLevel, 1_u32),
        .depth = 1
      };
    }

    auto copy = VkCopyBufferToImageInfo2(VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2);
    copy.srcBuffer = source.GetHandle();
    copy.dstImage = dest.GetHandle();
    copy.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copy.regionCount = 1;
    copy.pRegions = &region;
    vkCmdCopyBufferToImage2(_handle, &copy);
    return *this;
  }

  auto CCommandBuffer::ClearImage(const CImageView& imageView, const SClearValue& clearValue) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    auto aspectMask = imageView.GetAspectMask();
    const auto isColor = Core::IsFlagEnabled(aspectMask, EImageAspectFlag::E_COLOR);
    const auto isDepth = Core::IsFlagEnabled(aspectMask, EImageAspectFlag::E_DEPTH);
    const auto isStencil = Core::IsFlagEnabled(aspectMask, EImageAspectFlag::E_STENCIL);
    auto nativeClearValue = std::bit_cast<VkClearValue>(clearValue);
    auto subresourceRange = imageView.GetSubresourceRange();
    auto nativeSubresourceRange = VkImageSubresourceRange();
    nativeSubresourceRange.aspectMask = AsEnumCounterpart(aspectMask);
    nativeSubresourceRange.baseMipLevel = subresourceRange.BaseLevel;
    nativeSubresourceRange.levelCount = subresourceRange.LevelCount;
    nativeSubresourceRange.baseArrayLayer = subresourceRange.BaseLayer;
    nativeSubresourceRange.layerCount = subresourceRange.LayerCount;
    if (isColor) {
      vkCmdClearColorImage(
        _handle,
        imageView.GetImage().GetHandle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        &nativeClearValue.color,
        1,
        &nativeSubresourceRange
      );
    } else if (isDepth || isStencil) {
      vkCmdClearDepthStencilImage(
        _handle,
        imageView.GetImage().GetHandle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        &nativeClearValue.depthStencil,
        1,
        &nativeSubresourceRange
      );
    }
    return *this;
  }

  auto CCommandBuffer::CopyImage(const CImage& source, const CImage& dest, const SImageCopyRegion& copyRegion) noexcept -> CCommandBuffer& {
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

  auto CCommandBuffer::BlitImage(const CImage& source, const CImage& dest, const SImageBlitRegion& blitRegion) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    const auto sourceSubresource = MakeNativeImageSubresourceLayers(source, blitRegion.SourceSubresource);
    const auto destSubresource = MakeNativeImageSubresourceLayers(dest, blitRegion.DestSubresource);
    const auto sourceOffsets = std::bit_cast<std::array<VkOffset3D, 2>>(blitRegion.SourceOffsets);
    const auto destOffsets = std::bit_cast<std::array<VkOffset3D, 2>>(blitRegion.DestOffsets);

    auto region = VkImageBlit2(VK_STRUCTURE_TYPE_IMAGE_BLIT_2);
    region.srcSubresource = sourceSubresource;
    region.srcOffsets[0] = sourceOffsets[0];
    region.srcOffsets[1] = sourceOffsets[1];
    if (blitRegion.SourceOffsets[1] == SOffset3D()) {
      region.srcOffsets[1] = {
        .x = static_cast<int32>(source.GetWidth()),
        .y = static_cast<int32>(source.GetHeight()),
        .z = 1
      };
    }

    region.dstSubresource = destSubresource;
    region.dstOffsets[0] = destOffsets[0];
    region.dstOffsets[1] = destOffsets[1];
    if (blitRegion.DestOffsets[1] == SOffset3D()) {
      region.dstOffsets[1] = {
        .x = static_cast<int32>(dest.GetWidth()),
        .y = static_cast<int32>(dest.GetHeight()),
        .z = 1
      };
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

  auto CCommandBuffer::BeginNamedRegion(std::string_view name) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    auto region = VkDebugUtilsLabelEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT);
    region.pLabelName = name.data();
    vkCmdBeginDebugUtilsLabelEXT(_handle, &region);
    return *this;
  }

  auto CCommandBuffer::EndNamedRegion() noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    vkCmdEndDebugUtilsLabelEXT(_handle);
    return *this;
  }
}
