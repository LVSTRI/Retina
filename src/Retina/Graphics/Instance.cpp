#include <Retina/Graphics/Instance.hpp>
#include <Retina/Graphics/Macros.hpp>

#include <volk.h>

#include <algorithm>
#include <cstring>
#include <ranges>

namespace Retina::Graphics {
  namespace Details {
    RETINA_NODISCARD RETINA_INLINE auto EnumerateInstanceVersion() noexcept -> uint32 {
      RETINA_PROFILE_SCOPED();
      auto version = 0_u32;
      RETINA_GRAPHICS_VULKAN_CHECK(vkEnumerateInstanceVersion(&version));
      const auto instanceVersionMajor = VK_API_VERSION_MAJOR(version);
      const auto instanceVersionMinor = VK_API_VERSION_MINOR(version);
      const auto instanceVersionPatch = VK_API_VERSION_PATCH(version);
      if (version < VK_API_VERSION_1_3) {
        const auto expectedVersionMajor = VK_API_VERSION_MAJOR(VK_API_VERSION_1_3);
        const auto expectedVersionMinor = VK_API_VERSION_MINOR(VK_API_VERSION_1_3);
        const auto expectedVersionPatch = VK_API_VERSION_PATCH(VK_API_VERSION_1_3);
        RETINA_GRAPHICS_PANIC_WITH(
          "Instance version is too low (expected at least: {}.{}.{}, got: {}.{}.{})",
          expectedVersionMajor, expectedVersionMinor, expectedVersionPatch,
          instanceVersionMajor, instanceVersionMinor, instanceVersionPatch
        );
      }
      RETINA_GRAPHICS_INFO(
        "Instance version: {}.{}.{}",
        instanceVersionMajor,
        instanceVersionMinor,
        instanceVersionPatch
      );
      return version;
    }

    RETINA_NODISCARD RETINA_INLINE auto EnumerateInstanceExtensions() noexcept -> std::vector<VkExtensionProperties> {
      RETINA_PROFILE_SCOPED();
      auto count = 0_u32;
      RETINA_GRAPHICS_VULKAN_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
      auto extensions = std::vector<VkExtensionProperties>(count);
      RETINA_GRAPHICS_VULKAN_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()));
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

    RETINA_NODISCARD RETINA_INLINE auto GetEnabledInstanceExtensions(
      const SInstanceCreateInfo& createInfo,
      std::span<const VkExtensionProperties> extensions
    ) noexcept -> std::vector<const char*> {
      RETINA_PROFILE_SCOPED();
      auto enabledExtensions = std::vector<const char*>();
      if (createInfo.Features.Debug) {
        if (!IsExtensionAvailable(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
          RETINA_GRAPHICS_PANIC_WITH("Debug extension '{}' is not present", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        enabledExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      }

      if (createInfo.Features.Surface) {
        for (const auto& name : createInfo.GetSurfaceExtensionNames()) {
          if (!IsExtensionAvailable(extensions, name)) {
            RETINA_GRAPHICS_PANIC_WITH("Surface extension '{}' is not present", name);
          }
          enabledExtensions.emplace_back(name);
        }
      }
      return enabledExtensions;
    }
  }

  CInstance::~CInstance() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      vkDestroyInstance(_handle, nullptr);
      RETINA_GRAPHICS_INFO("Instance ({}) destroyed", static_cast<const void*>(_handle));
      volkFinalize();
      RETINA_GRAPHICS_INFO("Terminated graphics backend");
    }
  }

  auto CInstance::Make(const SInstanceCreateInfo& createInfo) noexcept -> Core::CArcPtr<CInstance> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CInstance());
    RETINA_GRAPHICS_VULKAN_CHECK(volkInitialize());
    RETINA_GRAPHICS_INFO("Initialized graphics backend");

    const auto version = Details::EnumerateInstanceVersion();
    const auto extensions = Details::EnumerateInstanceExtensions();
    const auto enabledExtensions = Details::GetEnabledInstanceExtensions(createInfo, extensions);

    auto applicationInfo = VkApplicationInfo(VK_STRUCTURE_TYPE_APPLICATION_INFO);
    applicationInfo.pApplicationName = "Retina";
    applicationInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    applicationInfo.pEngineName = "Retina";
    applicationInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_3;

    auto instanceCreateInfo = VkInstanceCreateInfo(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledExtensionCount = enabledExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

    auto instanceHandle = VkInstance();
    RETINA_GRAPHICS_VULKAN_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &instanceHandle));
    volkLoadInstanceOnly(instanceHandle);

    self->_handle = instanceHandle;
    self->_version = version;
    self->_createInfo = createInfo;
    return self;
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

  auto CInstance::IsFeatureEnabled(bool SInstanceFeature::* feature) const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Features.*feature;
  }
}
