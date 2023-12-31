#pragma once

#include <Retina/Core/Core.hpp>

#include <vulkan/vulkan.h>

#include <string>

namespace Retina {
    struct SPhysicalDeviceFeatures {
        VkPhysicalDeviceFeatures2 Features = {};
        VkPhysicalDeviceVulkan11Features Features11 = {};
        VkPhysicalDeviceVulkan12Features Features12 = {};
        VkPhysicalDeviceVulkan13Features Features13 = {};
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR RayTracingPipelineFeatures = {};
        VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR RayTracingPositionFetchFeatures = {};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR AccelerationStructureFeatures = {};
    };

    struct SDeviceRayTracingProperties {
        uint32 ShaderGroupHandleSize = 0;
        uint32 MaxRayRecursionDepth = 0;
        uint32 MaxShaderGroupStride = 0;
        uint32 ShaderGroupBaseAlignment = 0;
        uint32 ShaderGroupHandleCaptureReplaySize = 0;
        uint32 MaxRayDispatchInvocationCount = 0;
        uint32 ShaderGroupHandleAlignment = 0;
        uint32 MaxRayHitAttributeSize = 0;
    };

    struct SDeviceExtensionInfo {
        // TODO
        bool Swapchain = false;
        bool RayTracing = false;
        bool AccelerationStructure = false;

        RETINA_NODISCARD constexpr auto operator <=>(const SDeviceExtensionInfo&) const noexcept -> std::strong_ordering = default;
    };

    struct SDeviceFeatureInfo {
        // TODO

        RETINA_NODISCARD constexpr auto operator <=>(const SDeviceFeatureInfo&) const noexcept -> std::strong_ordering = default;
    };

    struct SDeviceCreateInfo {
        std::string Name;
        SDeviceExtensionInfo Extensions = {};
        SDeviceFeatureInfo Features = {};

        RETINA_NODISCARD constexpr auto operator <=>(const SDeviceCreateInfo&) const noexcept -> std::strong_ordering = default;
    };
}
