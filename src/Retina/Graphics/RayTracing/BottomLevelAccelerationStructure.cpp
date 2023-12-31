#include <Retina/Graphics/RayTracing/BottomLevelAccelerationStructure.hpp>

#include <Retina/Graphics/Buffer.hpp>
#include <Retina/Graphics/CommandBuffer.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/QueryPool.hpp>
#include <Retina/Graphics/Queue.hpp>

#include <volk.h>

namespace Retina {
    CBottomLevelAccelerationStructure::CBottomLevelAccelerationStructure() noexcept
        : IAccelerationStructure(EAccelerationStructureType::E_BOTTOM_LEVEL_KHR) {
        RETINA_PROFILE_SCOPED();
    }

    auto CBottomLevelAccelerationStructure::Make(
        const CDevice& device,
        const SAccelerationStructureCreateInfo& createInfo
    ) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto blas = CArcPtr(new Self());

        RETINA_LOG_INFO(device.GetLogger(), "Creating Bottom Level Acceleration Structure: \"{}\"", createInfo.Name);
        RETINA_LOG_INFO(device.GetLogger(), "- Flags: {}", ToString(createInfo.Flags));

        const auto geometryInfos = MakeNativeAccelerationStructureGeometryInfo(createInfo.GeometryInfos);
        const auto buildSizeInfo = MakeAccelerationStructureBuildSizeInfo(
            device,
            EAccelerationStructureType::E_BOTTOM_LEVEL_KHR,
            geometryInfos,
            createInfo
        );
        RETINA_LOG_INFO(device.GetLogger(), "- Size: {}", buildSizeInfo.AccelerationStructureSize);
        RETINA_LOG_INFO(device.GetLogger(), "- Build Scratch Size: {}", buildSizeInfo.BuildScratchSize);

        auto accelerationStructureBuffer = CBuffer::Make(device, {
            .Name = "BLAS_MainBuffer",
            .Capacity = buildSizeInfo.AccelerationStructureSize,
        });
        const auto accelerationStructureHandle = MakeAccelerationStructureHandle(
            device,
            *accelerationStructureBuffer,
            EAccelerationStructureType::E_BOTTOM_LEVEL_KHR
        );
        auto buildScratchBuffer = CBuffer::Make(device, {
            .Name = "BLAS_ScratchBuffer",
            .Capacity = buildSizeInfo.BuildScratchSize,
        });
        device.GetComputeQueue().Submit([&](CCommandBuffer& commands) noexcept {
            RETINA_PROFILE_SCOPED();
            auto buildInfo = SAccelerationStructureBuildInfo();
            buildInfo.Type = EAccelerationStructureType::E_BOTTOM_LEVEL_KHR;
            buildInfo.Flags = createInfo.Flags;
            buildInfo.Mode = EAccelerationStructureBuildMode::E_BUILD_KHR;
            buildInfo.Dest = accelerationStructureHandle;
            buildInfo.GeometryInfos = createInfo.GeometryInfos;
            buildInfo.ScratchBuffer = buildScratchBuffer;
            commands
                .MemoryBarrier({
                    .SourceStage = EPipelineStage::E_NONE,
                    .DestStage = EPipelineStage::E_ACCELERATION_STRUCTURE_BUILD_KHR,
                    .SourceAccess = EResourceAccess::E_NONE,
                    .DestAccess = EResourceAccess::E_ACCELERATION_STRUCTURE_WRITE_KHR,
                })
                .BuildAccelerationStructure(buildInfo)
                .MemoryBarrier({
                    .SourceStage = EPipelineStage::E_ACCELERATION_STRUCTURE_BUILD_KHR,
                    .DestStage = EPipelineStage::E_ACCELERATION_STRUCTURE_BUILD_KHR,
                    .SourceAccess = EResourceAccess::E_ACCELERATION_STRUCTURE_WRITE_KHR,
                    .DestAccess = EResourceAccess::E_ACCELERATION_STRUCTURE_READ_KHR,
                });
        });
        blas->_handle = accelerationStructureHandle;
        blas->_buffer = std::move(accelerationStructureBuffer);
        blas->_address = GetAccelerationStructureDeviceAddress(device, accelerationStructureHandle);
        blas->_device = device.ToArcPtr();
        blas->_createInfo = createInfo;
        blas->SetDebugName(createInfo.Name);

        return blas;
    }

    auto CBottomLevelAccelerationStructure::MakeCompact(
        const CDevice& device,
        const SAccelerationStructureCreateInfo& createInfo
    ) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto compactedBlas = CArcPtr(new Self());
        auto sourceBlas = Make(device, {
            .Name = "BLAS_Source",
            .Flags = createInfo.Flags | EAccelerationStructureBuildFlag::E_ALLOW_COMPACTION_KHR,
            .GeometryInfos = createInfo.GeometryInfos,
        });

        auto accelerationStructureQuery = CQueryPool::Make(device, {
            .Name = "BLAS_QueryPool",
            .Type = EQueryType::E_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
            .Count = 1,
        });
        device.GetComputeQueue().Submit([&](CCommandBuffer& commands) noexcept {
            RETINA_PROFILE_SCOPED();
            commands
                .ResetQueryPool(*accelerationStructureQuery, 0)
                .WriteAccelerationStructureProperties(*accelerationStructureQuery, *sourceBlas, 0);
        });
        const auto compactedSizeResults = *accelerationStructureQuery->FetchResult<uint64>(EQueryResultFlag::E_WAIT);
        RETINA_LOG_INFO(device.GetLogger(), "Compacting Bottom Level Acceleration Structure: \"{}\"", createInfo.Name);
        RETINA_LOG_INFO(device.GetLogger(), "- Size: {}", compactedSizeResults[0]);
        auto accelerationStructureBuffer = CBuffer::Make(device, {
            .Name = "BLAS_CompactedBuffer",
            .Capacity = compactedSizeResults[0],
        });
        const auto accelerationStructureHandle = MakeAccelerationStructureHandle(
            device,
            *accelerationStructureBuffer,
            EAccelerationStructureType::E_BOTTOM_LEVEL_KHR
        );
        device.GetComputeQueue().Submit([&](CCommandBuffer& commands) noexcept {
            RETINA_PROFILE_SCOPED();
            commands
                .MemoryBarrier({
                    .SourceStage = EPipelineStage::E_NONE,
                    .DestStage = EPipelineStage::E_ACCELERATION_STRUCTURE_BUILD_KHR,
                    .SourceAccess = EResourceAccess::E_NONE,
                    .DestAccess = EResourceAccess::E_ACCELERATION_STRUCTURE_WRITE_KHR,
                })
                .CopyAccelerationStructure({
                    .Source = sourceBlas->GetHandle(),
                    .Dest = accelerationStructureHandle,
                    .Mode = EAccelerationStructureCopyMode::E_COMPACT_KHR,
                })
                .MemoryBarrier({
                    .SourceStage = EPipelineStage::E_ACCELERATION_STRUCTURE_BUILD_KHR,
                    .DestStage = EPipelineStage::E_ACCELERATION_STRUCTURE_BUILD_KHR,
                    .SourceAccess = EResourceAccess::E_ACCELERATION_STRUCTURE_WRITE_KHR,
                    .DestAccess = EResourceAccess::E_ACCELERATION_STRUCTURE_READ_KHR,
                });
        });

        compactedBlas->_handle = accelerationStructureHandle;
        compactedBlas->_buffer = std::move(accelerationStructureBuffer);
        compactedBlas->_address = GetAccelerationStructureDeviceAddress(device, accelerationStructureHandle);
        compactedBlas->_device = device.ToArcPtr();
        compactedBlas->_createInfo = createInfo;
        compactedBlas->SetDebugName(createInfo.Name);

        return compactedBlas;
    }

    auto CBottomLevelAccelerationStructure::GetCreateInfo() const noexcept -> const SAccelerationStructureCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }
}
