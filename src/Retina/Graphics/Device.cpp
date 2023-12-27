#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/DeviceInfo.hpp>
#include <Retina/Graphics/Instance.hpp>
#include <Retina/Graphics/Queue.hpp>

#include <spdlog/sinks/stdout_color_sinks.h>

#include <volk.h>

#include <span>

#define RETINA_DEVICE_LOGGER_NAME "Device"

namespace Retina {
    RETINA_NODISCARD static auto EnumeratePhysicalDevices(const CInstance& instance) -> std::vector<VkPhysicalDevice> {
        RETINA_PROFILE_SCOPED();
        const auto logger = spdlog::get(RETINA_DEVICE_LOGGER_NAME);
        RETINA_LOG_INFO(*logger, "Enumerating Physical Devices");
        auto count = 0_u32;
        RETINA_VULKAN_CHECK(*logger, vkEnumeratePhysicalDevices(instance.GetHandle(), &count, nullptr));
        auto physicalDevices = std::vector<VkPhysicalDevice>(count);
        RETINA_VULKAN_CHECK(*logger, vkEnumeratePhysicalDevices(instance.GetHandle(), &count, physicalDevices.data()));
        return physicalDevices;
    }

    RETINA_NODISCARD static auto EnumeratePhysicalDeviceExtensions(VkPhysicalDevice physicalDevice) -> std::vector<VkExtensionProperties> {
        RETINA_PROFILE_SCOPED();
        const auto logger = spdlog::get(RETINA_DEVICE_LOGGER_NAME);
        RETINA_LOG_INFO(*logger, "Enumerating Physical Device Extensions");
        auto count = 0_u32;
        RETINA_VULKAN_CHECK(*logger, vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr));
        auto extensions = std::vector<VkExtensionProperties>(count);
        RETINA_VULKAN_CHECK(*logger, vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extensions.data()));
        return extensions;
    }

    RETINA_NODISCARD static auto IsExtensionAvailable(std::span<const VkExtensionProperties> extensions, const char* name) noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        const auto logger = spdlog::get(RETINA_DEVICE_LOGGER_NAME);
        for (const auto& extension : extensions) {
            if (std::strcmp(extension.extensionName, name) == 0) {
                return true;
            }
        }
        return false;
    }

    RETINA_NODISCARD static auto SelectPhysicalDevice(const CInstance& instance) noexcept -> VkPhysicalDevice {
        RETINA_PROFILE_SCOPED();
        const auto logger = spdlog::get(RETINA_DEVICE_LOGGER_NAME);
        RETINA_LOG_INFO(*logger, "Acquiring Physical Device");
        const auto physicalDevices = EnumeratePhysicalDevices(instance);
        for (const auto& physicalDevice : physicalDevices) {
            auto properties = VkPhysicalDeviceProperties();
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);
            if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU) {
                const auto driverVersionMajor = VK_API_VERSION_MAJOR(properties.apiVersion);
                const auto driverVersionMinor = VK_API_VERSION_MINOR(properties.apiVersion);
                const auto driverVersionPatch = VK_API_VERSION_PATCH(properties.apiVersion);
                RETINA_LOG_INFO(
                    *logger,
                    "Physical Device: {} - {}.{}.{}",
                    properties.deviceName,
                    driverVersionMajor,
                    driverVersionMinor,
                    driverVersionPatch
                );
                return physicalDevice;
            }
        }
        RETINA_PANIC_WITH(*logger, "No discrete GPU found in the system");
    }

    RETINA_NODISCARD static auto FetchPhysicalDeviceProperties(VkPhysicalDevice physicalDevice) noexcept -> SPhysicalDeviceProperties {
        RETINA_PROFILE_SCOPED();
        const auto logger = spdlog::get(RETINA_DEVICE_LOGGER_NAME);
        RETINA_LOG_INFO(*logger, "Fetching Physical Device Properties");

        auto properties = VkPhysicalDeviceProperties2(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2);
        auto memoryProperties = VkPhysicalDeviceMemoryProperties2(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2);
        auto rayTracingPipelineProperties = VkPhysicalDeviceRayTracingPipelinePropertiesKHR(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR);
        auto accelerationStructureProperties = VkPhysicalDeviceAccelerationStructurePropertiesKHR(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR);
        properties.pNext = &rayTracingPipelineProperties;
        rayTracingPipelineProperties.pNext = &accelerationStructureProperties;
        vkGetPhysicalDeviceProperties2(physicalDevice, &properties);
        vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &memoryProperties);

        return {
            properties,
            memoryProperties,
            rayTracingPipelineProperties,
            accelerationStructureProperties,
        };
    }

    RETINA_NODISCARD static auto FetchPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice) noexcept -> SPhysicalDeviceFeatures {
        RETINA_PROFILE_SCOPED();
        const auto logger = spdlog::get(RETINA_DEVICE_LOGGER_NAME);
        RETINA_LOG_INFO(*logger, "Enumerating Physical Devices");

        auto features = VkPhysicalDeviceFeatures2(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2);
        auto features11 = VkPhysicalDeviceVulkan11Features(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES);
        auto features12 = VkPhysicalDeviceVulkan12Features(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
        auto features13 = VkPhysicalDeviceVulkan13Features(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES);
        auto rayTracingPipelineFeatures = VkPhysicalDeviceRayTracingPipelineFeaturesKHR(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR);
        auto accelerationStructureFeatures = VkPhysicalDeviceAccelerationStructureFeaturesKHR(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR);

        features.pNext = &features11;
        features11.pNext = &features12;
        features12.pNext = &features13;
        features13.pNext = &rayTracingPipelineFeatures;
        rayTracingPipelineFeatures.pNext = &accelerationStructureFeatures;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &features);

        return {
            features,
            features11,
            features12,
            features13,
            rayTracingPipelineFeatures,
            accelerationStructureFeatures,
        };
    }

    RETINA_NODISCARD static auto MakeEnabledExtensions(const SDeviceExtensionInfo& extensionInfo, std::span<const VkExtensionProperties> availableExtensions) noexcept -> std::vector<const char*> {
        RETINA_PROFILE_SCOPED();
        const auto logger = spdlog::get(RETINA_DEVICE_LOGGER_NAME);
        RETINA_LOG_INFO(*logger, "Enabling Requested Physical Device Extensions");
        auto enabledExtensions = std::vector<const char*>();

#define RETINA_ENABLE_EXTENSION_OR_PANIC(x)                                             \
    do {                                                                                \
        if (!IsExtensionAvailable(availableExtensions, x)) {                            \
            RETINA_PANIC_WITH(*logger, "Required Extension \"{}\" not supported", x);   \
        }                                                                               \
        enabledExtensions.push_back(x);                                                 \
    } while (false);

        //RETINA_ENABLE_EXTENSION_OR_PANIC(VK_GOOGLE_HLSL_FUNCTIONALITY_1_EXTENSION_NAME);

        if (extensionInfo.Swapchain) {
            RETINA_ENABLE_EXTENSION_OR_PANIC(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

#undef RETINA_ENABLE_EXTENSION_OR_PANIC
        return enabledExtensions;
    }

    RETINA_NODISCARD static auto MakeEnabledFeatures(const SDeviceFeatureInfo& featureInfo, const SPhysicalDeviceFeatures& availableFeatures) noexcept -> SPhysicalDeviceFeatures {
        RETINA_PROFILE_SCOPED();
        const auto logger = spdlog::get(RETINA_DEVICE_LOGGER_NAME);
        RETINA_LOG_INFO(*logger, "Enabling Required and Requested Physical Device Features");
        auto enabledFeatures = SPhysicalDeviceFeatures(
            VkPhysicalDeviceFeatures2(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2),
            VkPhysicalDeviceVulkan11Features(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES),
            VkPhysicalDeviceVulkan12Features(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES),
            VkPhysicalDeviceVulkan13Features(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES),
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR),
            VkPhysicalDeviceAccelerationStructureFeaturesKHR(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR)
        );

        // Enable basic necessary features
#define RETINA_ENABLE_FEATURE_OR_PANIC(x)                                           \
    do {                                                                            \
        if (!availableFeatures.x) {                                                 \
            RETINA_PANIC_WITH(*logger, "Required Feature\"{}\" not supported", #x); \
        }                                                                           \
        enabledFeatures.x = true;                                                   \
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
        RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.shaderUniformBufferArrayDynamicIndexing);
        RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.shaderSampledImageArrayDynamicIndexing);
        RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.shaderStorageBufferArrayDynamicIndexing);
        RETINA_ENABLE_FEATURE_OR_PANIC(Features.features.shaderStorageImageArrayDynamicIndexing);
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
#undef RETINA_ENABLE_FEATURE_OR_PANIC

        return enabledFeatures;
    }

    RETINA_NODISCARD static auto MakeMainAllocator(const CDevice& device) noexcept -> VmaAllocator {
        RETINA_PROFILE_SCOPED();
        const auto logger = spdlog::get(RETINA_DEVICE_LOGGER_NAME);
        RETINA_LOG_INFO(*logger, "Creating Main Allocator");
        auto allocatorCreateInfo = VmaAllocatorCreateInfo();
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        allocatorCreateInfo.physicalDevice = device.GetPhysicalDevice();
        allocatorCreateInfo.device = device.GetHandle();
        allocatorCreateInfo.preferredLargeHeapBlockSize = 0;
        allocatorCreateInfo.pAllocationCallbacks = nullptr;
        allocatorCreateInfo.pDeviceMemoryCallbacks = nullptr;
        allocatorCreateInfo.pHeapSizeLimit = nullptr;
        allocatorCreateInfo.pVulkanFunctions = nullptr;
        allocatorCreateInfo.instance = device.GetInstance().GetHandle();
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;

        auto allocator = VmaAllocator();
        RETINA_VULKAN_CHECK(*logger, vmaCreateAllocator(&allocatorCreateInfo, &allocator));
        return allocator;
    }

    CDevice::~CDevice() noexcept {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(*_logger, "Terminating \"{}\"", GetDebugName());
        _graphicsQueue.Reset();
        _computeQueue.Reset();
        _transferQueue.Reset();
        vmaDestroyAllocator(_allocator);
        vkDestroyDevice(_handle, nullptr);
    }

    auto CDevice::Make(const CInstance& instance, const SDeviceCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto device = CArcPtr(new Self());
        auto logger = spdlog::stdout_color_mt(RETINA_DEVICE_LOGGER_NAME);

        RETINA_LOG_INFO(*logger, "Initializing Device \"{}\"", createInfo.Name);
        const auto physicalDevice = SelectPhysicalDevice(instance);
        const auto physicalDeviceProperties = FetchPhysicalDeviceProperties(physicalDevice);
        const auto physicalDeviceFeatures = FetchPhysicalDeviceFeatures(physicalDevice);
        const auto availableExtensions = EnumeratePhysicalDeviceExtensions(physicalDevice);
        const auto enabledExtensions = MakeEnabledExtensions(createInfo.Extensions, availableExtensions);

        RETINA_LOG_INFO(*logger, "Enumerating Queue Families");
        auto queueFamilyCount = 0_u32;
        vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, nullptr);
        auto queueFamilyProperties = std::vector(queueFamilyCount, VkQueueFamilyProperties2(VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2));
        vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

        auto queueFamilyCounts = std::vector<uint32>(queueFamilyCount);
        auto queuePriorities = std::vector<std::vector<float32>>(queueFamilyCount);
        const auto tryGetQueue = [&](
            VkQueueFlags requiredFlags,
            VkQueueFlags ignoredFlags,
            float32 priority
        ) noexcept -> std::optional<SQueueFamilyInfo> {
            for (auto i = 0_u32; i < queueFamilyCount; ++i) {
                const auto& properties = queueFamilyProperties[i].queueFamilyProperties;
                if ((properties.queueFlags & ignoredFlags) != 0) {
                    continue;
                }
                const auto remaining = properties.queueCount - queueFamilyCounts[i];
                if (remaining > 0 && (properties.queueFlags & requiredFlags) == requiredFlags) {
                    queuePriorities[i].emplace_back(priority);
                    return SQueueFamilyInfo {
                        .FamilyIndex = i,
                        .QueueIndex = queueFamilyCounts[i]++,
                    };
                }
            }
            return std::nullopt;
        };

        RETINA_LOG_INFO(*logger, "Selecting Queue Families");
        const auto graphicsFamily = tryGetQueue(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, 0, 1.0f);
        RETINA_ASSERT_WITH(graphicsFamily, "No graphics queue family found");
        auto computeFamily = tryGetQueue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT, 0, 0.5f);
        if (!computeFamily) {
            computeFamily = tryGetQueue(VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT, 0.5f);
            if (!computeFamily) {
                computeFamily = graphicsFamily;
            }
        }

        auto transferFamily = tryGetQueue(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 1.0f);
        if (!transferFamily) {
            transferFamily = tryGetQueue(VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT, 1.0f);
            if (!transferFamily) {
                transferFamily = computeFamily;
            }
        }

        auto queueCreateInfos = std::vector<VkDeviceQueueCreateInfo>();
        for (auto i = 0_u32; i < queueFamilyCount; ++i) {
            if (queueFamilyCounts[i] > 0) {
                auto queueCreateInfo = VkDeviceQueueCreateInfo(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
                queueCreateInfo.queueFamilyIndex = i;
                queueCreateInfo.queueCount = queueFamilyCounts[i];
                queueCreateInfo.pQueuePriorities = queuePriorities[i].data();
                queueCreateInfos.emplace_back(queueCreateInfo);
            }
        }

        auto enabledFeatures = MakeEnabledFeatures(createInfo.Features, physicalDeviceFeatures);
        enabledFeatures.Features.pNext = &enabledFeatures.Features11;
        enabledFeatures.Features11.pNext = &enabledFeatures.Features12;
        enabledFeatures.Features12.pNext = &enabledFeatures.Features13;
        enabledFeatures.Features13.pNext = &enabledFeatures.RayTracingPipelineFeatures;
        enabledFeatures.RayTracingPipelineFeatures.pNext = &enabledFeatures.AccelerationStructureFeatures;

        auto deviceCreateInfo = VkDeviceCreateInfo(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
        deviceCreateInfo.pNext = &enabledFeatures.Features;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32>(enabledExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

        auto deviceHandle = VkDevice();
        RETINA_VULKAN_CHECK(*logger, vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &deviceHandle));
        volkLoadDevice(deviceHandle);

        device->_handle = deviceHandle;
        device->_physicalDevice = physicalDevice;
        device->_physicalDeviceProperties = physicalDeviceProperties;
        device->_graphicsQueue = CQueue::Make(*device, SQueueCreateInfo {
            .Name = "GraphicsQueue",
            .Domain = EQueueDomain::E_GRAPHICS,
            .FamilyInfo = *graphicsFamily,
        });
        device->_computeQueue = [&] {
            if (computeFamily == graphicsFamily) {
                return device->_graphicsQueue;
            }
            return CQueue::Make(*device, SQueueCreateInfo {
                .Name = "ComputeQueue",
                .Domain = EQueueDomain::E_COMPUTE,
                .FamilyInfo = *computeFamily,
            });
        }();
        device->_transferQueue = [&] {
            if (transferFamily == graphicsFamily) {
                return device->_graphicsQueue;
            }
            if (transferFamily == computeFamily) {
                return device->_computeQueue;
            }
            return CQueue::Make(*device, SQueueCreateInfo {
                .Name = "TransferQueue",
                .Domain = EQueueDomain::E_TRANSFER,
                .FamilyInfo = *transferFamily,
            });
        }();
        device->_createInfo = createInfo;
        device->_logger = std::move(logger);
        device->_instance = instance.ToArcPtr();
        device->SetDebugName(createInfo.Name);

        device->_allocator = MakeMainAllocator(*device);

        return device;
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

    auto CDevice::GetCreateInfo() const noexcept -> const SDeviceCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CDevice::GetLogger() const noexcept -> spdlog::logger& {
        RETINA_PROFILE_SCOPED();
        return *_logger;
    }

    auto CDevice::GetInstance() const noexcept -> const CInstance& {
        RETINA_PROFILE_SCOPED();
        return *_instance;
    }

    auto CDevice::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_DEVICE;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(*_logger, vkSetDebugUtilsObjectNameEXT(_handle, &info));
    }

    auto CDevice::WaitIdle() const noexcept -> void {
        RETINA_PROFILE_SCOPED();
        RETINA_VULKAN_CHECK(*_logger, vkDeviceWaitIdle(_handle));
    }
}
