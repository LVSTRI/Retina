#include <Retina/Graphics/Instance.hpp>
#include <Retina/Graphics/InstanceInfo.hpp>

#include <spdlog/sinks/stdout_color_sinks.h>

#include <volk.h>

#include <algorithm>
#include <ranges>
#include <array>
#include <span>

#define RETINA_INSTANCE_LOGGER_NAME "Instance"

namespace Retina {
    RETINA_NODISCARD static auto GetInstanceVersion() noexcept -> uint32 {
        RETINA_PROFILE_SCOPED();
        auto instanceVersion = 0_u32;
        const auto logger = spdlog::get(RETINA_INSTANCE_LOGGER_NAME);
        RETINA_LOG_INFO(*logger, "Fetching Instance Version");
        RETINA_VULKAN_CHECK(*logger, vkEnumerateInstanceVersion(&instanceVersion));
        const auto instanceVersionMajor = VK_API_VERSION_MAJOR(instanceVersion);
        const auto instanceVersionMinor = VK_API_VERSION_MINOR(instanceVersion);
        const auto instanceVersionPatch = VK_API_VERSION_PATCH(instanceVersion);
        if (instanceVersion < VK_API_VERSION_1_3) {
            constexpr static auto expectedVersionMajor = VK_API_VERSION_MAJOR(VK_API_VERSION_1_3);
            constexpr static auto expectedVersionMinor = VK_API_VERSION_MINOR(VK_API_VERSION_1_3);
            constexpr static auto expectedVersionPatch = VK_API_VERSION_PATCH(VK_API_VERSION_1_3);
            RETINA_PANIC_WITH(
                *logger,
                "Instance Version (got: {}.{}.{}) is too low (expected: {}.{}.{})",
                instanceVersionMajor, instanceVersionMinor, instanceVersionPatch,
                expectedVersionMajor, expectedVersionMinor, expectedVersionPatch
            );
        }
        RETINA_LOG_INFO(*logger, "Instance Version: {}.{}.{}", instanceVersionMajor, instanceVersionMinor, instanceVersionPatch);
        return instanceVersion;
    }

    RETINA_NODISCARD static auto EnumerateInstanceExtensions() noexcept -> std::vector<VkExtensionProperties> {
        RETINA_PROFILE_SCOPED();
        const auto logger = spdlog::get(RETINA_INSTANCE_LOGGER_NAME);
        RETINA_LOG_INFO(*logger, "Enumerating Instance Extensions");
        auto count = 0_u32;
        RETINA_VULKAN_CHECK(*logger, vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
        auto extensionProperties = std::vector<VkExtensionProperties>(count);
        RETINA_VULKAN_CHECK(*logger, vkEnumerateInstanceExtensionProperties(nullptr, &count, extensionProperties.data()));
        return extensionProperties;
    }

    RETINA_NODISCARD static auto IsExtensionAvailable(std::span<const VkExtensionProperties> extensions, const char* name) noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        for (const auto& extension : extensions) {
            if (std::strcmp(extension.extensionName, name) == 0) {
                return true;
            }
        }
        return false;
    }

    RETINA_NODISCARD static auto MakeEnabledExtensions(
        std::span<const VkExtensionProperties> extensions,
        const SInstanceCreateInfo& createInfo
    ) noexcept -> std::vector<const char*> {
        RETINA_PROFILE_SCOPED();
        const auto logger = spdlog::get(RETINA_INSTANCE_LOGGER_NAME);
        RETINA_LOG_INFO(*logger, "Enabling Instance Extensions");
        auto enabledExtensions = std::vector<const char*>();
        if (createInfo.Features.Surface) {
            if (!IsExtensionAvailable(extensions, VK_KHR_SURFACE_EXTENSION_NAME)) {
                RETINA_PANIC_WITH(*logger, "Extension \"{}\" requested but not available", VK_KHR_SURFACE_EXTENSION_NAME);
            }
            const auto platformSurfaceExtensions = createInfo.PlatformGetSurfaceExtensionsFunc();
            enabledExtensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
            enabledExtensions.insert(enabledExtensions.end(), platformSurfaceExtensions.begin(), platformSurfaceExtensions.end());
        }

        if (createInfo.Features.Debug) {
            if (!IsExtensionAvailable(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
                RETINA_PANIC_WITH(*logger, "Extension \"{}\" requested but not available", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
            enabledExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return enabledExtensions;
    }

    CInstance::~CInstance() noexcept {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(*_logger, "Terminating Instance");
        vkDestroyInstance(_handle, nullptr);
        volkFinalize();
    }

    auto CInstance::Make(const SInstanceCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto instance = CArcPtr(new Self());
        auto logger = spdlog::stdout_color_mt(RETINA_INSTANCE_LOGGER_NAME);
        RETINA_LOG_INFO(*logger, "Initializing Main Instance");
        RETINA_VULKAN_CHECK(*logger, volkInitialize());

        const auto instanceVersion = GetInstanceVersion();
        const auto extensionProperties = EnumerateInstanceExtensions();
        const auto enabledExtensions = MakeEnabledExtensions(extensionProperties, createInfo);

        auto applicationInfo = VkApplicationInfo(VK_STRUCTURE_TYPE_APPLICATION_INFO);
        applicationInfo.pApplicationName = "Retina Application";
        applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        applicationInfo.pEngineName = "Retina Engine";
        applicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        applicationInfo.apiVersion = VK_API_VERSION_1_3;

        auto instanceCreateInfo = VkInstanceCreateInfo(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
        instanceCreateInfo.pApplicationInfo = &applicationInfo;
        instanceCreateInfo.enabledExtensionCount = static_cast<uint32>(enabledExtensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

        auto instanceHandle = VkInstance();
        RETINA_VULKAN_CHECK(*logger, vkCreateInstance(&instanceCreateInfo, nullptr, &instanceHandle));
        volkLoadInstanceOnly(instanceHandle);

        instance->_handle = instanceHandle;
        instance->_version = instanceVersion;
        instance->_createInfo = createInfo;
        instance->_logger = std::move(logger);

        return instance;
    }

    auto CInstance::GetHandle() const noexcept -> VkInstance {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto CInstance::GetVersion() const noexcept -> uint32 {
        RETINA_PROFILE_SCOPED();
        return _version;
    }

    auto CInstance::GetCreateInfo() const noexcept -> const SInstanceCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CInstance::GetLogger() const noexcept -> spdlog::logger& {
        RETINA_PROFILE_SCOPED();
        return *_logger;
    }

    auto CInstance::IsFeatureEnabled(bool SInstanceFeatureInfo::* feature) const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        return _createInfo.Features.*feature;
    }
}
