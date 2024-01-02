#pragma once

namespace Retina {
    // <Core/ArcPtr.hpp>
    template <typename T>
    class CArcPtr;

    // <Core/EnableIntrusiveReferenceCount.hpp>
    template <typename T>
    class IEnableIntrusiveReferenceCount;

    // <Core/Enums.hpp>
    enum class EImageLayout : uint32;
    enum class ENativeObjectType : uint32;
    enum class EResourceFormat : uint32;
    enum class EQueryType : uint32;
    enum class EComponentSwizzle : uint32;
    enum class EBlendFactor : uint32;
    enum class EBlendOperator : uint32;
    enum class ECompareOperator : uint32;
    enum class EDynamicState : uint32;
    enum class EFrontFace : uint32;
    enum class EPolygonMode : uint32;
    enum class EStencilOperator : uint32;
    enum class ELogicOperator : uint32;
    enum class ESamplerBorderColor : uint32;
    enum class ESamplerFilter : uint32;
    enum class ESamplerAddressMode : uint32;
    enum class ESamplerMipmapMode : uint32;
    enum class EDescriptorType : uint32;
    enum class EAttachmentLoadOperator : uint32;
    enum class EAttachmentStoreOperator : uint32;
    enum class EImageAspect : uint32;
    enum class EImageCreateFlag : uint32;
    enum class ESampleCount : uint32;
    enum class EImageUsage : uint32;
    enum class EColorComponent : uint32;
    enum class EShaderStage : uint32;
    enum class ECullMode : uint32;
    enum class EDescriptorPoolCreateFlag : uint32;
    enum class EDescriptorLayoutCreateFlag : uint32;
    enum class EMemoryProperty : uint32;
    enum class EQueryPipelineStatistic : uint32;
    enum class EQueryResultFlag : uint32;
    enum class EBufferCreateFlag : uint32;
    enum class ECommandPoolCreateFlag : uint32;
    enum class ESamplerReductionMode : uint32;
    enum class EDescriptorBindingFlag : uint32;
    enum class EPipelineStage : uint64;
    enum class EResourceAccess : uint64;
    enum class EPresentMode : uint32;
    enum class EColorSpace : uint32;
    enum class EAccelerationStructureType : uint32;
    enum class EAccelerationStructureCopyMode : uint32;
    enum class EAccelerationStructureGeometryFlag : uint32;
    enum class EAccelerationStructureGeometryInstanceFlag : uint32;
    enum class EAccelerationStructureBuildFlag : uint32;
    enum class EAccelerationStructureBuildMode : uint32;

    // <Graphics/Native/NativeDebugName.hpp>
    class INativeDebugName;

    // <Graphics/RayTracing/AccelerationStructure.hpp>
    class IAccelerationStructure;

    // <Graphics/RayTracing/AccelerationStructureInfo.hpp>
    struct SAccelerationStructureGeometryTrianglesData;
    struct SAccelerationStructureGeometryAabbsData;
    struct SAccelerationStructureGeometryInstance;
    struct SAccelerationStructureBuildSizeInfo;
    struct SAccelerationStructureGeometryInstancesData;
    struct SAccelerationStructureGeometryInfo;
    struct SAccelerationStructureBuildRangeInfo;
    struct SAccelerationStructureBuildInfo;
    struct SBottomLevelAccelerationStructureCreateInfo;

    // <Graphics/RayTracing/BottomLevelAccelerationStructure.hpp>
    class CBottomLevelAccelerationStructure;

    // <Graphics/RayTracing/RayTracingPipeline.hpp>
    struct SShaderBindingTable;
    class CRayTracingPipeline;

    // <Graphics/RayTracing/TopLevelAccelerationStructure.hpp>
    class CTopLevelAccelerationStructure;

    // <Graphics/Resources/DescriptorTable.hpp>
    template <EDescriptorType D>
    class CDescriptorTable;

    // <Graphics/Resources/ShaderResourceTable.hpp>
    template <EDescriptorType D, typename T = void>
    class CShaderResource;
    class CShaderResourceTable;

    // <Graphics/Sync/SyncHostTimeline.hpp>
    class CSyncHostDeviceTimeline;

    // <Graphics/BinarySemaphore.hpp>
    class CBinarySemaphore;

    // <Graphics/Buffer.hpp>
    class CBuffer;

    // <Graphics/BufferInfo.hpp>
    template <typename T = uint8>
    struct SBufferUploadInfo;
    struct SBufferCreateInfo;

    // <Graphics/CommandBuffer.hpp>
    class CCommandBuffer;

    // <Graphics/CommandBufferInfo.hpp>
    struct SDrawIndirectCommand;
    struct SDrawIndexedIndirectCommand;
    struct SDrawMeshTasksIndirectCommand;
    struct SDispatchIndirectCommand;
    struct SMemoryBarrier;
    struct SBufferMemoryBarrier;
    struct SImageMemoryBarrier;
    struct SBufferCopyRegion;
    struct SBufferImageCopyRegion;
    struct SImageCopyRegion;
    struct SImageBlitRegion;
    union SColorClearValue;
    struct SDepthStencilClearValue;
    union SAttachmentClearValue;
    struct SAttachmentInfo;
    struct SRenderingInfo;
    struct SCommandBufferCreateInfo;

    // <Graphics/CommandPool.hpp>
    class CCommandPool;

    // <Graphics/CommandPoolInfo.hpp>
    struct SCommandPoolCreateInfo;

    // <Graphics/ComputePipeline.hpp>
    class CComputePipeline;

    // <Graphics/DescriptorLayout.hpp>
    class CDescriptorLayout;

    // <Graphics/DescriptorLayoutInfo.hpp>
    struct SDescriptorLayoutBinding;
    struct SDescriptorLayoutCreateInfo;

    // <Graphics/DescriptorPool.hpp>
    class CDescriptorPool;

    // <Graphics/DescriptorPoolInfo.hpp>
    struct SDescriptorPoolSize;
    struct SDescriptorPoolCreateInfo;

    // <Graphics/DescriptorSet.hpp>
    class CDescriptorSet;

    // <Graphics/DescriptorSetInfo.hpp>
    struct SImageDescriptor;
    struct SBufferDescriptor;
    struct SAccelerationStructureDescriptor;
    struct SDescriptorWriteInfo;
    struct SDescriptorSetCreateInfo;

    // <Graphics/Device.hpp>
    class CDevice;

    // <Graphics/DeviceInfo.hpp>
    struct SPhysicalDeviceProperties;
    struct SPhysicalDeviceFeatures;
    struct SDeviceExtensionInfo;
    struct SDeviceFeatureInfo;
    struct SDeviceCreateInfo;

    // <Graphics/Fence.hpp>
    class CFence;

    // <Graphics/FenceInfo.hpp>
    struct SFenceCreateInfo;

    // <Graphics/GraphicsPipeline.hpp>
    class CGraphicsPipeline;

    // <Graphics/Image.hpp>
    class CImage;

    // <Graphics/ImageInfo.hpp>
    struct SImageSubresource;
    struct SImageSubresourceRange;
    struct SOffset2D;
    struct SExtent2D;
    struct SRect2D;
    struct SOffset3D;
    struct SExtent3D;
    struct SRect3D;
    struct SImageViewCreateInfo;
    struct SImageCreateInfo;

    // <Graphics/ImageView.hpp>
    class CImageView;

    // <Graphics/Instance.hpp>
    class CInstance;

    // <Graphics/InstanceInfo.hpp>
    struct SInstanceFeatureInfo;
    struct SInstanceCreateInfo;

    // <Graphics/Pipeline.hpp>
    class IPipeline;

    // <Graphics/PipelineInfo.hpp>
    enum class EPipelineType;
    struct SViewport;
    struct SScissor;
    struct SPipelineStencilState;
    struct SPipelineColorBlendAttachmentInfo;
    struct SPipelineTessellationStateInfo;
    struct SPipelineViewportStateInfo;
    struct SPipelineRasterizationStateInfo;
    struct SPipelineMultisampleStateInfo;
    struct SPipelineDepthStencilStateInfo;
    struct SPipelineColorBlendStateInfo;
    struct SPipelineDynamicStateInfo;
    struct SPipelineRenderingInfo;
    struct SPipelinePushConstantInfo;
    struct SPipelineLayout;
    struct SShaderBindingTableRegion;
    struct SComputePipelineCreateInfo;
    struct SGraphicsPipelineCreateInfo;
    struct SMeshShadingPipelineCreateInfo;
    struct SRayTracingPipelineCreateInfo;

    // <Graphics/QueryPool.hpp>
    class CQueryPool;

    // <Graphics/QueryPoolInfo.hpp>
    struct SQueryPoolCreateInfo;

    // <Graphics/Queue.hpp>
    class CQueue;

    // <Graphics/QueueInfo.hpp>
    enum class EQueueDomain;
    struct SQueueSemaphoreSubmitInfo;
    struct SQueueSubmitInfo;
    struct SQueuePresentInfo;
    struct SQueueFamilyInfo;
    struct SQueueCreateInfo;

    // <Graphics/Sampler.hpp>
    class CSampler;

    // <Graphics/SamplerInfo.hpp>
    struct SSamplerFilterInfo;
    struct SSamplerAddressInfo;
    struct SSamplerCreateInfo;

    // <Graphics/Semaphore.hpp>
    class ISemaphore;

    // <Graphics/SemaphoreInfo.hpp>
    struct SBinarySemaphoreCreateInfo;
    struct STimelineSemaphoreCreateInfo;
    enum class ESemaphoreKind;

    // <Graphics/Swapchain.hpp>
    class CSwapchain;

    // <Graphics/SwapchainInfo.hpp>
    struct SSwapchainCreateInfo;

    // <Graphics/TimelineSemaphore.hpp>
    class CTimelineSemaphore;

    // <Graphics/TypedBuffer.hpp>
    template <typename T>
    class CTypedBuffer;

    // <Platform/Input.hpp>
    class CInput;

    // <Platform/InputInfo.hpp>
    enum class EKeyboard : int32;

    // <Platform/Window.hpp>
    class CWindow;

    // <Platform/WindowInfo.hpp>
    struct SWindowFeatures;
    struct SWindowCreateInfo;
}
