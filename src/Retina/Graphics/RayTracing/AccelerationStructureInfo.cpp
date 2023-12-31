#include <Retina/Graphics/RayTracing/AccelerationStructureInfo.hpp>

#include <volk.h>

#include <vector>
#include <span>

namespace Retina {
    auto MakeNativeAccelerationStructureGeometryInfo(
        std::span<const SAccelerationStructureGeometryInfo> infos
    ) noexcept -> std::vector<VkAccelerationStructureGeometryKHR> {
        RETINA_PROFILE_SCOPED();
        auto geometryInfos = std::vector<VkAccelerationStructureGeometryKHR>();
        geometryInfos.reserve(infos.size());
        for (const auto& each : infos) {
            auto geometryInfo = VkAccelerationStructureGeometryKHR(VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR);
            geometryInfo.flags = ToEnumCounterpart(each.Flags);
            std::visit(
                SOverloadedVisitor {
                    [&](const SAccelerationStructureGeometryTrianglesData& data) {
                        constexpr static auto VERTEX_STRIDE = sizeof(float32[3]);
                        const auto maxVertexIndex = data.PositionBuffer->GetSize() / VERTEX_STRIDE - 1;
                        auto geometry = VkAccelerationStructureGeometryTrianglesDataKHR(
                            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR
                        );
                        geometry.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
                        geometry.vertexData.deviceAddress = data.PositionBuffer->GetAddress();
                        geometry.vertexStride = VERTEX_STRIDE;
                        geometry.maxVertex = maxVertexIndex;
                        geometry.indexType = VK_INDEX_TYPE_UINT32;
                        geometry.indexData.deviceAddress = data.IndexBuffer->GetAddress();

                        geometryInfo.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
                        geometryInfo.geometry.triangles = geometry;
                    },
                    [&](const SAccelerationStructureGeometryAabbsData& data) {
                        auto geometry = VkAccelerationStructureGeometryAabbsDataKHR(
                            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR
                        );
                        geometry.data.deviceAddress = data.AabbBuffer->GetAddress();
                        geometry.stride = sizeof(VkAabbPositionsKHR);

                        geometryInfo.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
                        geometryInfo.geometry.aabbs = geometry;
                    },
                    [&](const SAccelerationStructureGeometryInstancesData& data) {
                        auto geometry = VkAccelerationStructureGeometryInstancesDataKHR(
                            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR
                        );
                        geometry.arrayOfPointers = false;
                        geometry.data.deviceAddress = data.InstanceBuffer->GetAddress();

                        geometryInfo.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
                        geometryInfo.geometry.instances = geometry;
                    },
                },
                each.Data
            );
            geometryInfos.emplace_back(geometryInfo);
        }
        return geometryInfos;
    }

    auto MakeAccelerationStructureBuildSizeInfo(
        const CDevice& device,
        EAccelerationStructureType type,
        std::span<const VkAccelerationStructureGeometryKHR> geometryInfos,
        const SAccelerationStructureCreateInfo& createInfo
    ) noexcept -> SAccelerationStructureBuildSizeInfo {
        RETINA_PROFILE_SCOPED();
        auto primitiveCounts = std::vector<uint32>();
        primitiveCounts.reserve(geometryInfos.size());
        for (const auto& geometryInfo : createInfo.GeometryInfos) {
            primitiveCounts.push_back(geometryInfo.Range.PrimitiveCount);
        }

        auto buildGeometryInfo = VkAccelerationStructureBuildGeometryInfoKHR(VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR);
        buildGeometryInfo.type = ToEnumCounterpart(type);
        buildGeometryInfo.flags = ToEnumCounterpart(createInfo.Flags);
        buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildGeometryInfo.geometryCount = geometryInfos.size();
        buildGeometryInfo.pGeometries = geometryInfos.data();

        auto buildSizesInfo = VkAccelerationStructureBuildSizesInfoKHR(VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR);
        vkGetAccelerationStructureBuildSizesKHR(
            device.GetHandle(),
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &buildGeometryInfo,
            primitiveCounts.data(),
            &buildSizesInfo
        );
        return {
            .AccelerationStructureSize = buildSizesInfo.accelerationStructureSize,
            .UpdateScratchSize = buildSizesInfo.updateScratchSize,
            .BuildScratchSize = buildSizesInfo.buildScratchSize
        };
    }

    auto GetAccelerationStructureDeviceAddress(
        const CDevice& device,
        VkAccelerationStructureKHR accelerationStructure
    ) noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        auto accelerationStructureDeviceAddressInfo =
            VkAccelerationStructureDeviceAddressInfoKHR(VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR);
        accelerationStructureDeviceAddressInfo.accelerationStructure = accelerationStructure;
        return vkGetAccelerationStructureDeviceAddressKHR(device.GetHandle(), &accelerationStructureDeviceAddressInfo);
    }
}
