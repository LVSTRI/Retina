#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Instance.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Queue.hpp>
#include <Retina/Graphics/Macros.hpp>

#include <volk.h>

#include <compare>
#include <cstring>
#include <span>
#include <utility>
#include <vector>

namespace Retina::Graphics {
  namespace Details {
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
      VkPhysicalDeviceMeshShaderFeaturesEXT MeshShaderFeatures = {};
      VkPhysicalDeviceRayTracingPipelineFeaturesKHR RayTracingPipelineFeatures = {};
      VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR RayTracingPositionFetchFeatures = {};
      VkPhysicalDeviceAccelerationStructureFeaturesKHR AccelerationStructureFeatures = {};
    };

    struct SQueueFamilyInfo {
      uint32 Family = 0;
      uint32 Index = 0;

      RETINA_NODISCARD constexpr auto operator <=>(const SQueueFamilyInfo&) const noexcept -> std::strong_ordering = default;
    };

    struct SDeviceQueueInfo {
      std::vector<uint32> Counts;
      std::vector<std::vector<float32>> Priorities;
      std::tuple<SQueueFamilyInfo, SQueueFamilyInfo, SQueueFamilyInfo> Families;
    };

    RETINA_NODISCARD RETINA_INLINE auto EnumeratePhysicalDevices(
      VkInstance instance
    ) noexcept -> std::vector<VkPhysicalDevice> {
      RETINA_PROFILE_SCOPED();
      auto count = 0_u32;
      RETINA_GRAPHICS_VULKAN_CHECK(vkEnumeratePhysicalDevices(instance, &count, nullptr));
      auto physicalDevices = std::vector<VkPhysicalDevice>(count);
      RETINA_GRAPHICS_VULKAN_CHECK(vkEnumeratePhysicalDevices(instance, &count, physicalDevices.data()));
      RETINA_GRAPHICS_INFO("Acquired system physical devices");
      return physicalDevices;
    }

    RETINA_NODISCARD RETINA_INLINE auto EnumeratePhysicalDeviceExtensions(
      VkPhysicalDevice physicalDevice
    ) noexcept -> std::vector<VkExtensionProperties> {
      RETINA_PROFILE_SCOPED();
      auto count = 0_u32;
      RETINA_GRAPHICS_VULKAN_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr));
      auto extensions = std::vector<VkExtensionProperties>(count);
      RETINA_GRAPHICS_VULKAN_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extensions.data()));
      RETINA_GRAPHICS_INFO("Acquired physical device extensions");
      return extensions;
    }

    RETINA_NODISCARD RETINA_INLINE auto IsExtensionAvailable(
      std::span<const VkExtensionProperties> extensions,
      const char* name
    ) noexcept -> bool {
      RETINA_PROFILE_SCOPED();
      return std::ranges::find_if(extensions, [name](const auto& extension) {
        return std::strcmp(extension.extensionName, name) == 0;
      }) != extensions.end();
    }

    RETINA_NODISCARD RETINA_INLINE auto SelectPhysicalDevice(const CInstance& instance) noexcept -> VkPhysicalDevice {
      RETINA_PROFILE_SCOPED();
      auto physicalDevices = EnumeratePhysicalDevices(instance.GetHandle());
      RETINA_ASSERT_WITH(!physicalDevices.empty(), "No physical devices found");
      for (const auto& physicalDevice : physicalDevices) {
        auto properties = VkPhysicalDeviceProperties();
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU) {
          const auto driverVersionMajor = VK_API_VERSION_MAJOR(properties.apiVersion);
          const auto driverVersionMinor = VK_API_VERSION_MINOR(properties.apiVersion);
          const auto driverVersionPatch = VK_API_VERSION_PATCH(properties.apiVersion);
          RETINA_GRAPHICS_INFO(
            "Selected physical device: {} - {}.{}.{}",
            properties.deviceName,
            driverVersionMajor,
            driverVersionMinor,
            driverVersionPatch
          );
          return physicalDevice;
        }
      }
      RETINA_GRAPHICS_PANIC_WITH("No suitable physical device found");
    }

    RETINA_NODISCARD RETINA_INLINE auto GetPhysicalDeviceProperties(
      VkPhysicalDevice physicalDevice
    ) noexcept -> SPhysicalDeviceProperties {
      RETINA_PROFILE_SCOPED();
      auto properties = VkPhysicalDeviceProperties2(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2);
      auto memoryProperties = VkPhysicalDeviceMemoryProperties2(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2);
      auto rayTracingPipelineProperties = VkPhysicalDeviceRayTracingPipelinePropertiesKHR(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR);
      auto accelerationStructureProperties = VkPhysicalDeviceAccelerationStructurePropertiesKHR(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR);
      properties.pNext = &rayTracingPipelineProperties;
      rayTracingPipelineProperties.pNext = &accelerationStructureProperties;
      vkGetPhysicalDeviceProperties2(physicalDevice, &properties);
      vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &memoryProperties);
      RETINA_GRAPHICS_INFO("Acquired physical device properties");
      return {
        properties,
        memoryProperties,
        rayTracingPipelineProperties,
        accelerationStructureProperties,
      };
    }

    RETINA_NODISCARD RETINA_INLINE auto GetPhysicalDeviceFeatures(
      VkPhysicalDevice physicalDevice
    ) noexcept -> SPhysicalDeviceFeatures {
      RETINA_PROFILE_SCOPED();
      auto features = VkPhysicalDeviceFeatures2(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2);
      auto features11 = VkPhysicalDeviceVulkan11Features(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES);
      auto features12 = VkPhysicalDeviceVulkan12Features(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
      auto features13 = VkPhysicalDeviceVulkan13Features(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES);
      auto meshShaderFeatures = VkPhysicalDeviceMeshShaderFeaturesEXT(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT);
      auto rayTracingPipelineFeatures = VkPhysicalDeviceRayTracingPipelineFeaturesKHR(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR);
      auto rayTracingPositionFetchFeatures = VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR);
      auto accelerationStructureFeatures = VkPhysicalDeviceAccelerationStructureFeaturesKHR(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR);
      features.pNext = &features11;
      features11.pNext = &features12;
      features12.pNext = &features13;
      features13.pNext = &meshShaderFeatures;
      meshShaderFeatures.pNext = &rayTracingPipelineFeatures;
      rayTracingPipelineFeatures.pNext = &rayTracingPositionFetchFeatures;
      rayTracingPositionFetchFeatures.pNext = &accelerationStructureFeatures;
      vkGetPhysicalDeviceFeatures2(physicalDevice, &features);

      RETINA_GRAPHICS_INFO("Acquired physical device features");
      return {
        features,
        features11,
        features12,
        features13,
        meshShaderFeatures,
        rayTracingPipelineFeatures,
        rayTracingPositionFetchFeatures,
        accelerationStructureFeatures,
      };
    }

    RETINA_NODISCARD RETINA_INLINE auto GetPhysicalDeviceQueueProperties(
      VkPhysicalDevice physicalDevice
    ) noexcept -> std::vector<VkQueueFamilyProperties> {
      RETINA_PROFILE_SCOPED();
      auto count = 0_u32;
      vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
      auto queueFamilyProperties = std::vector<VkQueueFamilyProperties>(count);
      vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, queueFamilyProperties.data());
      RETINA_GRAPHICS_INFO("Acquired physical device queue family properties");
      return queueFamilyProperties;
    }

    RETINA_NODISCARD RETINA_INLINE auto MakeDeviceRayTracingProperties(
      const SPhysicalDeviceProperties& properties
    ) noexcept -> SDeviceRayTracingProperties {
      RETINA_PROFILE_SCOPED();
      return {
        properties.RayTracingPipelineProperties.shaderGroupHandleSize,
        properties.RayTracingPipelineProperties.maxRayRecursionDepth,
        properties.RayTracingPipelineProperties.maxShaderGroupStride,
        properties.RayTracingPipelineProperties.shaderGroupBaseAlignment,
        properties.RayTracingPipelineProperties.shaderGroupHandleCaptureReplaySize,
        properties.RayTracingPipelineProperties.maxRayDispatchInvocationCount,
        properties.RayTracingPipelineProperties.shaderGroupHandleAlignment,
        properties.RayTracingPipelineProperties.maxRayHitAttributeSize,
      };
    }

    RETINA_NODISCARD RETINA_INLINE auto GetEnabledDeviceExtensions(
      const SDeviceFeature& features,
      std::span<const VkExtensionProperties> extensions
    ) noexcept -> std::vector<const char*> {
      auto enabledExtensions = std::vector<const char*>();

#define RETINA_ENABLE_EXTENSION_OR_PANIC(x)                                     \
  do {                                                                          \
    if (!IsExtensionAvailable(extensions, x)) {                                 \
      RETINA_GRAPHICS_PANIC_WITH("Required extension \"{}\" not supported", x); \
    }                                                                           \
    enabledExtensions.emplace_back(x);                                          \
  } while (false)

      if (features.Swapchain) {
        RETINA_ENABLE_EXTENSION_OR_PANIC(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
      }
      if (features.MeshShader) {
        RETINA_ENABLE_EXTENSION_OR_PANIC(VK_EXT_MESH_SHADER_EXTENSION_NAME);
      }
      if (features.RayTracingPipeline) {
        RETINA_ENABLE_EXTENSION_OR_PANIC(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        RETINA_ENABLE_EXTENSION_OR_PANIC(VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME);
      }
      if (features.AccelerationStructure) {
        RETINA_ENABLE_EXTENSION_OR_PANIC(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        RETINA_ENABLE_EXTENSION_OR_PANIC(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
      }
#undef RETINA_ENABLE_EXTENSION_OR_PANIC

      RETINA_GRAPHICS_INFO("Enabled device extensions");
      return enabledExtensions;
    }

    RETINA_NODISCARD RETINA_INLINE auto GetEnabledDeviceFeatures(
      const SDeviceFeature& requestedFeatures,
      const SPhysicalDeviceFeatures& availableFeatures
    ) noexcept -> SPhysicalDeviceFeatures {
      RETINA_PROFILE_SCOPED();
      auto enabledFeatures = SPhysicalDeviceFeatures(
        VkPhysicalDeviceFeatures2(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2),
        VkPhysicalDeviceVulkan11Features(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES),
        VkPhysicalDeviceVulkan12Features(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES),
        VkPhysicalDeviceVulkan13Features(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES),
        VkPhysicalDeviceMeshShaderFeaturesEXT(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT),
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR),
        VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR),
        VkPhysicalDeviceAccelerationStructureFeaturesKHR(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR)
      );
      enabledFeatures.Features.pNext = &enabledFeatures.Features11;
      enabledFeatures.Features11.pNext = &enabledFeatures.Features12;
      enabledFeatures.Features12.pNext = &enabledFeatures.Features13;
      enabledFeatures.Features13.pNext = &enabledFeatures.MeshShaderFeatures;
      enabledFeatures.MeshShaderFeatures.pNext = &enabledFeatures.RayTracingPipelineFeatures;
      enabledFeatures.RayTracingPipelineFeatures.pNext = &enabledFeatures.RayTracingPositionFetchFeatures;
      enabledFeatures.RayTracingPositionFetchFeatures.pNext = &enabledFeatures.AccelerationStructureFeatures;

#define RETINA_ENABLE_FEATURE_OR_PANIC(x)                                         \
  do {                                                                            \
    if (!availableFeatures.x) {                                                   \
      RETINA_GRAPHICS_PANIC_WITH("Required feature \"{}\" is not supported", #x); \
    }                                                                             \
    enabledFeatures.x = true;                                                     \
  } while (false)

      // 1.0 Features
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.samplerAnisotropy);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.fullDrawIndexUint32);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.imageCubeArray);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.independentBlend);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.sampleRateShading);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.geometryShader);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.multiDrawIndirect);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.depthClamp);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.depthBiasClamp);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.depthBounds);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.wideLines);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.samplerAnisotropy);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.textureCompressionBC);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.pipelineStatisticsQuery);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.vertexPipelineStoresAndAtomics);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.fragmentStoresAndAtomics);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.shaderStorageImageMultisample);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.shaderStorageImageReadWithoutFormat);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.shaderStorageImageWriteWithoutFormat);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.shaderInt64);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.shaderInt16);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.shaderResourceResidency);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.shaderResourceMinLod);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.sparseBinding);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.sparseResidencyBuffer);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.sparseResidencyImage2D);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.sparseResidencyAliased);

      // 1.1 Features
      RETINA_ENABLE_FEATURE_OR_PANIC(Features11.storageBuffer16BitAccess);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features11.uniformAndStorageBuffer16BitAccess);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features11.multiview);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features11.variablePointersStorageBuffer);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features11.variablePointers);

      // 1.2 Features
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.drawIndirectCount);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.storageBuffer8BitAccess);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.uniformAndStorageBuffer8BitAccess);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.shaderBufferInt64Atomics);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.shaderFloat16);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.shaderInt8);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.descriptorIndexing);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.shaderUniformBufferArrayNonUniformIndexing);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.shaderSampledImageArrayNonUniformIndexing);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.shaderStorageBufferArrayNonUniformIndexing);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.shaderStorageImageArrayNonUniformIndexing);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.descriptorBindingUniformBufferUpdateAfterBind);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.descriptorBindingSampledImageUpdateAfterBind);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.descriptorBindingStorageImageUpdateAfterBind);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.descriptorBindingStorageBufferUpdateAfterBind);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.descriptorBindingUpdateUnusedWhilePending);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.descriptorBindingPartiallyBound);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.runtimeDescriptorArray);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.samplerFilterMinmax);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.scalarBlockLayout);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.imagelessFramebuffer);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.shaderSubgroupExtendedTypes);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.hostQueryReset);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.timelineSemaphore);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.bufferDeviceAddress);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.bufferDeviceAddressCaptureReplay);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.vulkanMemoryModel);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.vulkanMemoryModelDeviceScope);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.shaderOutputViewportIndex);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.shaderOutputLayer);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features12.subgroupBroadcastDynamicId);

      // 1.3 Features
      RETINA_ENABLE_FEATURE_OR_PANIC(Features13.subgroupSizeControl);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features13.computeFullSubgroups);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features13.synchronization2);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features13.dynamicRendering);
      RETINA_ENABLE_FEATURE_OR_PANIC(Features13.maintenance4);

      // Extension Features
      if (requestedFeatures.MeshShader) {
        RETINA_ENABLE_FEATURE_OR_PANIC(MeshShaderFeatures.taskShader);
        RETINA_ENABLE_FEATURE_OR_PANIC(MeshShaderFeatures.meshShader);
      }

      if (requestedFeatures.RayTracingPipeline) {
        RETINA_ENABLE_FEATURE_OR_PANIC(RayTracingPipelineFeatures.rayTracingPipeline);
        RETINA_ENABLE_FEATURE_OR_PANIC(RayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect);
        RETINA_ENABLE_FEATURE_OR_PANIC(RayTracingPipelineFeatures.rayTraversalPrimitiveCulling);
        RETINA_ENABLE_FEATURE_OR_PANIC(RayTracingPositionFetchFeatures.rayTracingPositionFetch);
      }

      if (requestedFeatures.AccelerationStructure) {
        RETINA_ENABLE_FEATURE_OR_PANIC(AccelerationStructureFeatures.accelerationStructure);
        RETINA_ENABLE_FEATURE_OR_PANIC(AccelerationStructureFeatures.accelerationStructureCaptureReplay);
        RETINA_ENABLE_FEATURE_OR_PANIC(AccelerationStructureFeatures.descriptorBindingAccelerationStructureUpdateAfterBind);
      }

      RETINA_GRAPHICS_INFO("Enabled device features");
#undef RETINA_ENABLE_FEATURE_OR_PANIC
      return enabledFeatures;
    }

    RETINA_NODISCARD RETINA_INLINE auto MakeAllocator(
      VkInstance instance,
      VkDevice device,
      VkPhysicalDevice physicalDevice
    ) noexcept -> VmaAllocator {
      RETINA_PROFILE_SCOPED();
      auto allocatorCreateInfo = VmaAllocatorCreateInfo();
      allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
      allocatorCreateInfo.physicalDevice = physicalDevice;
      allocatorCreateInfo.device = device;
      allocatorCreateInfo.preferredLargeHeapBlockSize = 0;
      allocatorCreateInfo.pAllocationCallbacks = nullptr;
      allocatorCreateInfo.pDeviceMemoryCallbacks = nullptr;
      allocatorCreateInfo.pHeapSizeLimit = nullptr;
      allocatorCreateInfo.pVulkanFunctions = nullptr;
      allocatorCreateInfo.instance = instance;
      allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;

      auto allocator = VmaAllocator();
      RETINA_GRAPHICS_VULKAN_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &allocator));
      return allocator;
    }

    RETINA_NODISCARD RETINA_INLINE auto MakeDeviceQueueInfos(
      std::span<const VkQueueFamilyProperties> queueFamilyProperties
    ) noexcept -> SDeviceQueueInfo {
      RETINA_PROFILE_SCOPED();
      auto queueFamilyCounts = std::vector<uint32>(queueFamilyProperties.size());
      auto queueFamilyPriorities = std::vector<std::vector<float32>>(queueFamilyProperties.size());
      const auto tryGetQueue = [&](
        VkQueueFlags required,
        VkQueueFlags ignored,
        float32 priority
      ) noexcept -> std::optional<SQueueFamilyInfo> {
        for (auto i = 0_u32; i < queueFamilyProperties.size(); ++i) {
          const auto& properties = queueFamilyProperties[i];
          if (properties.queueFlags & ignored) {
            continue;
          }

          const auto remaining = properties.queueCount - queueFamilyCounts[i];
          if (remaining > 0 && Core::IsFlagEnabled(properties.queueFlags, required)) {
            queueFamilyPriorities[i].emplace_back(priority);
            return SQueueFamilyInfo(i, queueFamilyCounts[i]++);
          }
        }
        return std::nullopt;
      };

      const auto graphicsQueueFamily = tryGetQueue(
        VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT,
        0,
        1.0f
      );
      RETINA_ASSERT_WITH(graphicsQueueFamily, "No graphics queue family found");

      auto computeQueueFamily = tryGetQueue(VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT, 0.5f);
      if (!computeQueueFamily) {
        computeQueueFamily = tryGetQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_TRANSFER_BIT, 0.5f);
        if (!computeQueueFamily) {
          computeQueueFamily = graphicsQueueFamily;
        }
      }

      auto transferQueueFamily = tryGetQueue(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 1.0f);
      if (!transferQueueFamily) {
        transferQueueFamily = tryGetQueue(VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT, 1.0f);
        if (!transferQueueFamily) {
          transferQueueFamily = graphicsQueueFamily;
        }
      }

      RETINA_GRAPHICS_INFO("Acquired device queues");

      return {
        std::move(queueFamilyCounts),
        std::move(queueFamilyPriorities),
        std::forward_as_tuple(
          *graphicsQueueFamily,
          *computeQueueFamily,
          *transferQueueFamily
        )
      };
    }
  }

  CDevice::~CDevice() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      _deletionQueue->Flush();
      _transferQueue.Reset();
      _computeQueue.Reset();
      _graphicsQueue.Reset();
      vmaDestroyAllocator(_allocator);
      vkDestroyDevice(_handle, nullptr);
      RETINA_GRAPHICS_INFO("Device ({}) destroyed", GetDebugName());
    }
  }

  auto CDevice::Make(
    const CInstance& instance,
    const SDeviceCreateInfo& createInfo
  ) noexcept -> Core::CArcPtr<CDevice> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CDevice());
    const auto physicalDevice = Details::SelectPhysicalDevice(instance);
    const auto physicalDeviceProperties = Details::GetPhysicalDeviceProperties(physicalDevice);
    const auto physicalDeviceFeatures = Details::GetPhysicalDeviceFeatures(physicalDevice);
    const auto physicalDeviceQueues = Details::GetPhysicalDeviceQueueProperties(physicalDevice);
    const auto availableDeviceExtensions = Details::EnumeratePhysicalDeviceExtensions(physicalDevice);
    const auto enabledDeviceExtensions = Details::GetEnabledDeviceExtensions(createInfo.Features, availableDeviceExtensions);
    const auto enabledDeviceFeatures = Details::GetEnabledDeviceFeatures(createInfo.Features, physicalDeviceFeatures);
    const auto deviceRayTracingProperties = Details::MakeDeviceRayTracingProperties(physicalDeviceProperties);
    const auto deviceQueueInfos = Details::MakeDeviceQueueInfos(physicalDeviceQueues);
    const auto [
      graphicsQueueFamily,
      computeQueueFamily,
      transferQueueFamily
    ] = deviceQueueInfos.Families;

    auto deviceQueueCreateInfo = std::vector<VkDeviceQueueCreateInfo>();
    for (auto i = 0_u32; i < physicalDeviceQueues.size(); ++i) {
      if (deviceQueueInfos.Counts[i] > 0) {
        auto queueCreateInfo = VkDeviceQueueCreateInfo(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
        queueCreateInfo.queueFamilyIndex = i;
        queueCreateInfo.queueCount = deviceQueueInfos.Counts[i];
        queueCreateInfo.pQueuePriorities = deviceQueueInfos.Priorities[i].data();
        deviceQueueCreateInfo.emplace_back(queueCreateInfo);
      }
    }

    auto deviceCreateInfo = VkDeviceCreateInfo(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
    deviceCreateInfo.queueCreateInfoCount = deviceQueueCreateInfo.size();
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfo.data();
    deviceCreateInfo.enabledExtensionCount = enabledDeviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = enabledDeviceExtensions.data();
    deviceCreateInfo.pNext = &enabledDeviceFeatures.Features;

    auto deviceHandle = VkDevice();
    RETINA_GRAPHICS_VULKAN_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &deviceHandle));
    volkLoadDevice(deviceHandle);
    RETINA_GRAPHICS_INFO("Device ({}) initialized", createInfo.Name);

    auto allocator = Details::MakeAllocator(instance.GetHandle(), deviceHandle, physicalDevice);

    self->_handle = deviceHandle;
    self->_physicalDevice = physicalDevice;
    self->_allocator = allocator;
    self->_deletionQueue = CDeletionQueue::Make();
    self->_rayTracingProperties = deviceRayTracingProperties;
    self->_createInfo = createInfo;
    self->_instance = instance.ToArcPtr();

    self->_graphicsQueue = CQueue::Make(*self, {
      .Name = "GraphicsQueue",
      .Domain = EQueueDomain::E_GRAPHICS,
      .FamilyIndex = graphicsQueueFamily.Family,
      .QueueIndex = graphicsQueueFamily.Index,
    });
    self->_computeQueue = self->_graphicsQueue;
    self->_transferQueue = self->_graphicsQueue;
    if (computeQueueFamily != graphicsQueueFamily) {
      self->_computeQueue = CQueue::Make(*self, {
        .Name = "ComputeQueue",
        .Domain = EQueueDomain::E_COMPUTE,
        .FamilyIndex = computeQueueFamily.Family,
        .QueueIndex = computeQueueFamily.Index,
      });
    }
    if (transferQueueFamily != graphicsQueueFamily) {
      self->_transferQueue = CQueue::Make(*self, {
        .Name = "TransferQueue",
        .Domain = EQueueDomain::E_TRANSFER,
        .FamilyIndex = transferQueueFamily.Family,
        .QueueIndex = transferQueueFamily.Index,
      });
    }
    self->SetDebugName(createInfo.Name);

    return self;
  }

  auto CDevice::GetHandle() const noexcept -> VkDevice {
    RETINA_PROFILE_SCOPED();
    return _handle;
  }

  auto CDevice::GetPhysicalDevice() const noexcept -> VkPhysicalDevice {
    RETINA_PROFILE_SCOPED();
    return _physicalDevice;
  }

  auto CDevice::GetAllocator() const noexcept -> VmaAllocator {
    RETINA_PROFILE_SCOPED();
    return _allocator;
  }

  auto CDevice::GetGraphicsQueue() const noexcept -> CQueue& {
    RETINA_PROFILE_SCOPED();
    return *_graphicsQueue;
  }

  auto CDevice::GetComputeQueue() const noexcept -> CQueue& {
    RETINA_PROFILE_SCOPED();
    return *_computeQueue;
  }

  auto CDevice::GetTransferQueue() const noexcept -> CQueue& {
    RETINA_PROFILE_SCOPED();
    return *_transferQueue;
  }

  auto CDevice::GetDeletionQueue() const noexcept -> CDeletionQueue& {
    RETINA_PROFILE_SCOPED();
    return *_deletionQueue;
  }

  auto CDevice::GetCreateInfo() const noexcept -> const SDeviceCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CDevice::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CDevice::GetInstance() const noexcept -> const CInstance& {
    RETINA_PROFILE_SCOPED();
    return *_instance;
  }

  auto CDevice::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_DEBUG_NAME(_handle, _handle, VK_OBJECT_TYPE_DEVICE, name);
    _createInfo.Name = name;
  }

  auto CDevice::IsFeatureEnabled(bool SDeviceFeature::* feature) const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Features.*feature;
  }

  auto CDevice::WaitIdle() const noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_VULKAN_CHECK(vkDeviceWaitIdle(_handle));
  }

  auto CDevice::Tick() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _deletionQueue->Tick();
  }
}
