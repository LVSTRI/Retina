#include <Retina/Graphics/Resources/ShaderResourceTable.hpp>
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

    auto depthStencilAttachment = VkRenderingAttachmentInfo();
    if (renderingInfo.DepthStencilAttachment) {
      const auto& attachmentInfo = *renderingInfo.DepthStencilAttachment;
      const auto& imageView = *attachmentInfo.ImageView;
      const auto& image = imageView.GetImage();
      const auto subresource = imageView.GetSubresourceRange();
      depthStencilAttachment = Details::MakeNativeRenderingAttachmentInfo(
        attachmentInfo, EImageLayout::E_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
      );
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
    if (renderingInfo.DepthStencilAttachment) {
      commandRenderingInfo.pDepthAttachment = &depthStencilAttachment;
      commandRenderingInfo.pStencilAttachment = &depthStencilAttachment;
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
    vkCmdPushConstants(_handle, layout.Handle, AsEnumCounterpart(currentPipeline.GetBindPoint()), offset, values.size_bytes(), values.data());
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

  auto CCommandBuffer::Barrier(const SMemoryBarrierInfo& barrierInfo) noexcept -> CCommandBuffer& {
    RETINA_PROFILE_SCOPED();
    auto memoryBarriers = std::vector<VkMemoryBarrier2>();
    memoryBarriers.reserve(barrierInfo.MemoryBarriers.size());
    for (const auto& barrier : barrierInfo.MemoryBarriers) {
      memoryBarriers.emplace_back(Details::MakeNativeMemoryBarrier(barrier));
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

  auto CCommandBuffer::CopyImage(const CImage& source, const CImage& dest, SImageCopyRegion copyRegion) noexcept -> CCommandBuffer& {
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

  auto CCommandBuffer::BlitImage(const CImage& source, const CImage& dest, SImageBlitRegion blitRegion) noexcept -> CCommandBuffer& {
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
