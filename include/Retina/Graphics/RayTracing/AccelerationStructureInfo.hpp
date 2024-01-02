#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/TypedBuffer.hpp>

#include <glm/vec3.hpp>

#include <variant>

namespace Retina {
    struct SAccelerationStructureGeometryTrianglesData {
        const CTypedBuffer<glm::vec3>* PositionBuffer = nullptr;
        const CTypedBuffer<uint32>* IndexBuffer = nullptr;
    };

    struct SAccelerationStructureGeometryAabbsData {
        const CTypedBuffer<VkAabbPositionsKHR>* AabbBuffer = nullptr;
    };

    struct SAccelerationStructureGeometryInstance {
        VkTransformMatrixKHR Transform = {};
        uint32 ObjectIndex : 24 = 0;
        uint32 Mask : 8 = 0xff;
        uint32 ShaderBindingTableOffset : 24 = 0;
        EAccelerationStructureGeometryInstanceFlag Flags : 8 = {};
        uint64 AccelerationStructureAddress = 0;
    };
    static_assert(sizeof(SAccelerationStructureGeometryInstance) == sizeof(VkAccelerationStructureInstanceKHR));

    struct SAccelerationStructureBuildSizeInfo {
        uint64 AccelerationStructureSize = 0;
        uint64 UpdateScratchSize = 0;
        uint64 BuildScratchSize = 0;
    };

    struct SAccelerationStructureGeometryInstancesData {
        const CTypedBuffer<SAccelerationStructureGeometryInstance>* InstanceBuffer = nullptr;
    };

    using AccelerationStructureGeometryData = std::variant<
        SAccelerationStructureGeometryTrianglesData,
        SAccelerationStructureGeometryAabbsData,
        SAccelerationStructureGeometryInstancesData
    >;

    struct SAccelerationStructureBuildRangeInfo {
        uint32 PrimitiveCount = 0;
        uint32 PrimitiveOffset = 0;
        uint32 FirstVertex = 0;
        uint32 TransformOffset = 0;
    };

    struct SAccelerationStructureGeometryInfo {
        SAccelerationStructureBuildRangeInfo Range = {};
        EAccelerationStructureGeometryFlag Flags = {};
        AccelerationStructureGeometryData Data = {};
    };

    struct SAccelerationStructureBuildInfo {
        EAccelerationStructureType Type = {};
        EAccelerationStructureBuildFlag Flags = {};
        EAccelerationStructureBuildMode Mode = {};
        VkAccelerationStructureKHR Source = {};
        VkAccelerationStructureKHR Dest = {};
        CArcPtr<const CBuffer> ScratchBuffer;
        std::vector<SAccelerationStructureGeometryInfo> GeometryInfos;
    };

    struct SAccelerationStructureCreateInfo {
        std::string Name;
        EAccelerationStructureBuildFlag Flags = {};
        std::vector<SAccelerationStructureGeometryInfo> GeometryInfos;
    };

    RETINA_NODISCARD auto MakeNativeAccelerationStructureGeometryInfo(
        std::span<const SAccelerationStructureGeometryInfo> infos
    ) noexcept -> std::vector<VkAccelerationStructureGeometryKHR>;

    RETINA_NODISCARD auto MakeAccelerationStructureBuildSizeInfo(
        const CDevice& device,
        EAccelerationStructureType type,
        std::span<const VkAccelerationStructureGeometryKHR> geometryInfos,
        const SAccelerationStructureCreateInfo& createInfo
    ) noexcept -> SAccelerationStructureBuildSizeInfo;

    RETINA_NODISCARD auto GetAccelerationStructureDeviceAddress(
        const CDevice& device,
        VkAccelerationStructureKHR accelerationStructure
    ) noexcept -> uint64;
}
