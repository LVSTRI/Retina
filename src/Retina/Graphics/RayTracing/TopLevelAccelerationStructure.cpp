#include <Retina/Graphics/RayTracing/TopLevelAccelerationStructure.hpp>
#include <Retina/Graphics/CommandBuffer.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Queue.hpp>

namespace Retina {
    CTopLevelAccelerationStructure::CTopLevelAccelerationStructure() noexcept
        : IAccelerationStructure(EAccelerationStructureType::E_TOP_LEVEL_KHR) {
        RETINA_PROFILE_SCOPED();
    }

    auto CTopLevelAccelerationStructure::Make(
        const CDevice& device,
        const SAccelerationStructureCreateInfo& createInfo
    ) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto tlas = CArcPtr(new Self());

        RETINA_LOG_INFO(device.GetLogger(), "Creating Top Level Acceleration Structure: \"{}\"", createInfo.Name);
        RETINA_LOG_INFO(device.GetLogger(), "- Flags: {}", ToString(createInfo.Flags));

        const auto geometryInfos = MakeNativeAccelerationStructureGeometryInfo(createInfo.GeometryInfos);
        const auto buildSizeInfo = MakeAccelerationStructureBuildSizeInfo(
            device,
            EAccelerationStructureType::E_TOP_LEVEL_KHR,
            geometryInfos,
            createInfo
        );
        RETINA_LOG_INFO(device.GetLogger(), "- Size: {}", buildSizeInfo.AccelerationStructureSize);
        RETINA_LOG_INFO(device.GetLogger(), "- Build Scratch Size: {}", buildSizeInfo.BuildScratchSize);

        auto accelerationStructureBuffer = CBuffer::Make(device, {
            .Name = "TLAS_MainBuffer",
            .Capacity = buildSizeInfo.AccelerationStructureSize,
        });
        const auto accelerationStructureHandle = MakeAccelerationStructureHandle(
            device,
            *accelerationStructureBuffer,
            EAccelerationStructureType::E_TOP_LEVEL_KHR
        );

        auto buildScratchBuffer = CBuffer::Make(device, {
            .Name = "TLAS_ScratchBuffer",
            .Capacity = buildSizeInfo.BuildScratchSize,
        });
        device.GetComputeQueue().Submit([&](CCommandBuffer& commands) noexcept {
            RETINA_PROFILE_SCOPED();
            auto buildInfo = SAccelerationStructureBuildInfo();
            buildInfo.Type = EAccelerationStructureType::E_TOP_LEVEL_KHR;
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
                    .DestStage = EPipelineStage::E_RAY_TRACING_SHADER_KHR,
                    .SourceAccess = EResourceAccess::E_ACCELERATION_STRUCTURE_WRITE_KHR,
                    .DestAccess = EResourceAccess::E_ACCELERATION_STRUCTURE_READ_KHR,
                });
        });

        tlas->_handle = accelerationStructureHandle;
        tlas->_buffer = std::move(accelerationStructureBuffer);
        tlas->_createInfo = createInfo;
        tlas->_device = device.ToArcPtr();
        tlas->SetDebugName(createInfo.Name);

        return tlas;
    }

    auto CTopLevelAccelerationStructure::GetCreateInfo() const noexcept -> const SAccelerationStructureCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }
}
