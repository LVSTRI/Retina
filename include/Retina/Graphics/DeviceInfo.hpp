#pragma once

#include <Retina/Core/Core.hpp>

#include <vulkan/vulkan.h>

#include <string>

namespace Retina {
    struct SPhysicalDeviceProperties {
        VkPhysicalDeviceProperties2 Properties = {};
        VkPhysicalDeviceMemoryProperties2 MemoryProperties = {};
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR RayTracingPipelineProperties = {};
        VkPhysicalDeviceAccelerationStructurePropertiesKHR AccelerationStructureProperties = {};
    };

    struct SPhysicalDeviceFeatures {
        VkPhysicalDeviceFeatures2 Features = {};
        VkPhysicalDeviceVulkan11Features Features11 = {};
        VkPhysicalDeviceVulkan12Features Features12 = {};
        VkPhysicalDeviceVulkan13Features Features13 = {};
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR RayTracingPipelineFeatures = {};
        VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR RayTracingPositionFetchFeatures = {};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR AccelerationStructureFeatures = {};
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
