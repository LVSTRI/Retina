#pragma once

namespace Retina::Graphics {
  // <Retina/Graphics/BinarySemaphore.hpp>
  class CBinarySemaphore;

  // <Retina/Graphics/CommandBuffer.hpp>
  class CCommandBuffer;

  // <Retina/Graphics/CommandBufferInfo.hpp>
  struct SDrawIndirectCommand;
  struct SDrawIndexedIndirectCommand;
  struct SDrawMeshTasksIndirectCommand;
  struct SDispatchIndirectCommand;
  struct SMemoryBarrier;
  struct SImageMemoryBarrier;
  struct SMemoryBarrierInfo;
  struct SImageCopyRegion;
  struct SImageBlitRegion;
  union SColorClearValue;
  struct SDepthStencilClearValue;
  union SAttachmentClearValue;
  struct SAttachmentInfo;
  struct SRenderingInfo;
  struct SCommandBufferCreateInfo;

  // <Retina/Graphics/CommandPool.hpp>
  class CCommandPool;

  // <Retina/Graphics/CommandPoolInfo.hpp>
  struct SCommandPoolCreateInfo;

  // <Retina/Graphics/Device.hpp>
  class CDevice;

  // <Retina/Graphics/DeviceInfo.hpp>
  struct SDeviceRayTracingProperties;
  struct SDeviceFeature;
  struct SDeviceCreateInfo;

  // <Retina/Graphics/Fence.hpp>
  class CFence;

  // <Retina/Graphics/FenceInfo.hpp>
  struct SFenceCreateInfo;

  // <Retina/Graphics/HostDeviceTimeline.hpp>
  class CHostDeviceTimeline;

  // <Retina/Graphics/Image.hpp>
  class CImage;

  // <Retina/Graphics/ImageInfo.hpp>
  struct SImageSubresource;
  struct SImageSubresourceRange;
  struct SOffset2D;
  struct SExtent2D;
  struct SRect2D;
  struct SOffset3D;
  struct SExtent3D;
  struct SRect3D;
  struct SImageSwizzle;
  struct SImageViewCreateInfo;
  struct SImageCreateInfo;

  // <Retina/Graphics/ImageView.hpp>
  class CImageView;

  // <Retina/Graphics/Instance.hpp>
  class CInstance;

  // <Retina/Graphics/InstanceInfo.hpp>
  struct SInstanceFeature;
  struct SInstanceCreateInfo;

  // <Retina/Graphics/Queue.hpp>
  class CQueue;

  // <Retina/Graphics/QueueInfo.hpp>
  enum class EQueueDomain;
  struct SQueueCreateInfo;
  struct SQueueSemaphoreSubmitInfo;

  // <Retina/Graphics/Semaphore.hpp>
  class ISemaphore;

  // <Retina/Graphics/SemaphoreInfo.hpp>
  enum class ESemaphoreKind;
  struct SBinarySemaphoreCreateInfo;
  struct STimelineSemaphoreCreateInfo;

  // <Retina/Graphics/Swapchain.hpp>
  class CSwapchain;

  // <Retina/Graphics/SwapchainInfo.hpp>
  struct SSwapchainCreateInfo;
  struct SSwapchainPresentInfo;

  // <Retina/Graphics/TimelineSemaphore.hpp>
  class CTimelineSemaphore;
}
