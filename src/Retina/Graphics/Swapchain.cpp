#include <Retina/Graphics/BinarySemaphore.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Instance.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/ImageView.hpp>
#include <Retina/Graphics/Queue.hpp>
#include <Retina/Graphics/Swapchain.hpp>

#include <Retina/Platform/Window.hpp>

#include <volk.h>

namespace Retina {
    RETINA_NODISCARD static auto IsSurfaceSupported(const CDevice& device, EQueueDomain domain, VkSurfaceKHR surface) noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(device.GetLogger(), "Checking Surface Support");
        auto supported = VkBool32();
        const auto familyIndex = [&] {
            switch (domain) {
                case EQueueDomain::E_GRAPHICS: return device.GetGraphicsQueue().GetFamilyIndex();
                case EQueueDomain::E_COMPUTE: return device.GetComputeQueue().GetFamilyIndex();
                case EQueueDomain::E_TRANSFER: return device.GetTransferQueue().GetFamilyIndex();
            }
        }();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkGetPhysicalDeviceSurfaceSupportKHR(
                device.GetPhysicalDevice(),
                familyIndex,
                surface,
                &supported
            )
        );
        return supported;
    }

    RETINA_NODISCARD static auto GetSurfaceCapabilities(const CDevice& device, VkSurfaceKHR surface) noexcept -> VkSurfaceCapabilitiesKHR {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(device.GetLogger(), "Fetching Surface Capabilities");
        auto capabilities = VkSurfaceCapabilitiesKHR();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                device.GetPhysicalDevice(),
                surface,
                &capabilities
            )
        );
        return capabilities;
    }

    RETINA_NODISCARD static auto GetSurfaceFormats(const CDevice& device, VkSurfaceKHR surface) noexcept -> std::vector<VkSurfaceFormatKHR> {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(device.GetLogger(), "Fetching Surface Formats");
        auto count = 0_u32;
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                device.GetPhysicalDevice(),
                surface,
                &count,
                nullptr
            )
        );
        auto formats = std::vector<VkSurfaceFormatKHR>(count);
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                device.GetPhysicalDevice(),
                surface,
                &count,
                formats.data()
            )
        );
        return formats;
    }

    RETINA_NODISCARD static auto GetSurfacePresentModes(const CDevice& device, VkSurfaceKHR surface) noexcept -> std::vector<VkPresentModeKHR> {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(device.GetLogger(), "Fetching Surface Present Modes");
        auto count = 0_u32;
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device.GetPhysicalDevice(),
                surface,
                &count,
                nullptr
            )
        );
        auto presentModes = std::vector<VkPresentModeKHR>(count);
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device.GetPhysicalDevice(),
                surface,
                &count,
                presentModes.data()
            )
        );
        return presentModes;
    }

    RETINA_NODISCARD static auto GetSwapchainImageCount(const CDevice& device, const VkSurfaceCapabilitiesKHR& capabilities) noexcept -> uint32 {
        RETINA_PROFILE_SCOPED();
        constexpr static auto targetImageCount = 3_u32;
        const auto minImageCount = static_cast<uint32>(capabilities.minImageCount ? capabilities.minImageCount : targetImageCount);
        const auto maxImageCount = static_cast<uint32>(capabilities.maxImageCount ? capabilities.maxImageCount : targetImageCount);
        const auto imageCount = std::clamp(targetImageCount, minImageCount, maxImageCount);
        RETINA_LOG_INFO(device.GetLogger(), "- Image Count: {}", imageCount);
        return imageCount;
    }

    RETINA_NODISCARD static auto GetSurfaceColorSpace(const CDevice& device, std::span<const VkSurfaceFormatKHR> formats) noexcept -> VkColorSpaceKHR {
        RETINA_PROFILE_SCOPED();
        RETINA_ASSERT_WITH(!formats.empty(), "Surface has no Formats");
        const auto targetColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        for (const auto& format : formats) {
            if (format.colorSpace == targetColorSpace) {
                RETINA_LOG_INFO(device.GetLogger(), "- Color Space: {}", ToString(format.colorSpace));
                return targetColorSpace;
            }
        }
        RETINA_PANIC_WITH(device.GetLogger(), "Surface has no suitable Color Space");
    }

    RETINA_NODISCARD static auto GetSurfaceFormat(const CDevice& device, std::span<const VkSurfaceFormatKHR> formats) noexcept -> VkFormat {
        RETINA_PROFILE_SCOPED();
        RETINA_ASSERT_WITH(!formats.empty(), "Surface has no Formats");
        const auto targetFormats = std::to_array({
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_FORMAT_B8G8R8A8_UNORM,
        });
        for (const auto& targetFormat : targetFormats) {
            const auto isPresent = std::ranges::any_of(
                formats,
                [&](const auto& format) {
                    return format.format == targetFormat;
                }
            );
            if (isPresent) {
                RETINA_LOG_INFO(device.GetLogger(), "- Format: {}", ToString(targetFormat));
                return targetFormat;
            }
        }
        RETINA_PANIC_WITH(device.GetLogger(), "Surface has no suitable Format");
    }

    RETINA_NODISCARD static auto GetSurfacePresentMode(
        const CDevice& device,
        const SSwapchainCreateInfo& createInfo,
        std::span<const VkPresentModeKHR> presentModes
    ) noexcept -> VkPresentModeKHR {
        RETINA_PROFILE_SCOPED();
        RETINA_ASSERT_WITH(!presentModes.empty(), "Surface has no Present Modes");
        const auto targetPresentMode = createInfo.VSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
        for (const auto& presentMode : presentModes) {
            if (presentMode == targetPresentMode) {
                RETINA_LOG_INFO(device.GetLogger(), "- Present Mode: {}", ToString(presentMode));
                return targetPresentMode;
            }
        }
        RETINA_PANIC_WITH(device.GetLogger(), "Surface has no suitable Present Mode");
    }

    RETINA_NODISCARD static auto GetSurfaceCurrentExtent(
        const CDevice& device,
        const CWindow& window,
        const VkSurfaceCapabilitiesKHR& capabilities
    ) noexcept -> VkExtent2D {
        RETINA_PROFILE_SCOPED();
        if (capabilities.currentExtent.width != UINT32_MAX && capabilities.currentExtent.height != UINT32_MAX) {
            RETINA_LOG_INFO(
                device.GetLogger(),
                "- Extent: {}x{}",
                capabilities.currentExtent.width,
                capabilities.currentExtent.height
            );
            return capabilities.currentExtent;
        }
        const auto minExtent = VkExtent2D(
            capabilities.minImageExtent.width == UINT32_MAX ? 0 : capabilities.minImageExtent.width,
            capabilities.minImageExtent.height == UINT32_MAX ? 0 : capabilities.minImageExtent.height
        );
        const auto maxExtent = VkExtent2D(
            capabilities.maxImageExtent.width == UINT32_MAX ? 0 : capabilities.maxImageExtent.width,
            capabilities.maxImageExtent.height == UINT32_MAX ? 0 : capabilities.maxImageExtent.height
        );
        const auto width = std::clamp(
            static_cast<uint32>(window.GetWidth()),
            minExtent.width,
            maxExtent.width
        );
        const auto height = std::clamp(
            static_cast<uint32>(window.GetHeight()),
            minExtent.height,
            maxExtent.height
        );
        RETINA_LOG_INFO(device.GetLogger(), "- Extent: {}x{}", width, height);
        return { width, height };
    }

    CSwapchain::~CSwapchain() noexcept {
        RETINA_PROFILE_SCOPED();
        _images.clear();
        RETINA_LOG_INFO(_device->GetLogger(), "Terminating Swapchain \"{}\"", GetDebugName());
        vkDestroySwapchainKHR(_device->GetHandle(), _handle, nullptr);
        vkDestroySurfaceKHR(_device->GetInstance().GetHandle(), _surface, nullptr);
    }

    auto CSwapchain::Make(const CDevice& device, const CWindow& window, const SSwapchainCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto swapchain = CArcPtr(new Self());
        RETINA_LOG_INFO(device.GetLogger(), "Initializing Swapchain \"{}\"", createInfo.Name);

        const auto surface = reinterpret_cast<VkSurfaceKHR>(createInfo.MakeSurfaceFunc(device.GetInstance(), window));
        RETINA_ASSERT_WITH(
            IsSurfaceSupported(device, EQueueDomain::E_GRAPHICS, surface),
            "Surface is not Supported"
        );
        const auto surfaceCapabilities = GetSurfaceCapabilities(device, surface);
        const auto surfaceFormats = GetSurfaceFormats(device, surface);
        const auto surfacePresentModes = GetSurfacePresentModes(device, surface);
        const auto imageCount = GetSwapchainImageCount(device, surfaceCapabilities);
        const auto colorSpace = GetSurfaceColorSpace(device, surfaceFormats);
        const auto format = GetSurfaceFormat(device, surfaceFormats);
        const auto presentMode = GetSurfacePresentMode(device, createInfo, surfacePresentModes);
        const auto currentExtent = GetSurfaceCurrentExtent(device, window, surfaceCapabilities);
        const auto familyIndex = device.GetGraphicsQueue().GetFamilyIndex();

        auto swapchainCreateInfo = VkSwapchainCreateInfoKHR(VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
        swapchainCreateInfo.surface = surface;
        swapchainCreateInfo.minImageCount = imageCount;
        swapchainCreateInfo.imageFormat = format;
        swapchainCreateInfo.imageColorSpace = colorSpace;
        swapchainCreateInfo.imageExtent = currentExtent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                         VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 1;
        swapchainCreateInfo.pQueueFamilyIndices = &familyIndex;
        swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = presentMode;
        swapchainCreateInfo.clipped = true;

        auto swapchainHandle = VkSwapchainKHR();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkCreateSwapchainKHR(
                device.GetHandle(),
                &swapchainCreateInfo,
                nullptr,
                &swapchainHandle
            )
        );
        swapchain->_handle = swapchainHandle;
        swapchain->_surface = surface;
        swapchain->_width = currentExtent.width;
        swapchain->_height = currentExtent.height;
        swapchain->_format = ToEnumCounterpart(format);
        swapchain->_createInfo = createInfo;
        swapchain->_device = device.ToArcPtr();
        swapchain->_window = window.ToArcPtr();
        swapchain->SetDebugName(createInfo.Name);

        swapchain->_images = CImage::FromSwapchain(device, *swapchain, {
            .Name = "SwapchainImage",
            .Width = swapchain->_width,
            .Height = swapchain->_height,
            .Levels = 1,
            .Layers = 1,
            .Usage = EImageUsage::E_COLOR_ATTACHMENT |
                     EImageUsage::E_TRANSFER_DST,
            .Format = swapchain->_format,
            .ViewInfo = Constant::DEFAULT_IMAGE_VIEW_INFO
        });
        return swapchain;
    }

    auto CSwapchain::Recreate(
        CArcPtr<Self>&& oldSwapchain,
        const SSwapchainCreateInfo* createInfo
    ) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        const auto& device = oldSwapchain->GetDevice();
        const auto& window = oldSwapchain->GetWindow();
        auto newCreateInfo = oldSwapchain->GetCreateInfo();
        if (createInfo) {
            newCreateInfo = *createInfo;
        }
        RETINA_LOG_INFO(device.GetLogger(), "Recreating Swapchain \"{}\"", newCreateInfo.Name);
        oldSwapchain.Reset();
        return Make(device, window, newCreateInfo);
    }

    auto CSwapchain::GetHandle() const noexcept -> VkSwapchainKHR {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto CSwapchain::GetSurface() const noexcept -> VkSurfaceKHR {
        RETINA_PROFILE_SCOPED();
        return _surface;
    }

    auto CSwapchain::GetWidth() const noexcept -> uint32 {
        RETINA_PROFILE_SCOPED();
        return _width;
    }

    auto CSwapchain::GetHeight() const noexcept -> uint32 {
        RETINA_PROFILE_SCOPED();
        return _height;
    }

    auto CSwapchain::GetFormat() const noexcept -> EResourceFormat {
        RETINA_PROFILE_SCOPED();
        return _format;
    }

    auto CSwapchain::GetImages() const noexcept -> std::span<const CArcPtr<CImage>> {
        RETINA_PROFILE_SCOPED();
        return _images;
    }

    auto CSwapchain::GetCurrentImageIndex() const noexcept -> uint32 {
        RETINA_PROFILE_SCOPED();
        return _currentImageIndex;
    }

    auto CSwapchain::GetCreateInfo() const noexcept -> const SSwapchainCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CSwapchain::IsLost() const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        return _isLost;
    }

    auto CSwapchain::GetDevice() const noexcept -> const CDevice& {
        RETINA_PROFILE_SCOPED();
        return *_device;
    }

    auto CSwapchain::GetWindow() const noexcept -> const CWindow& {
        RETINA_PROFILE_SCOPED();
        return *_window;
    }

    auto CSwapchain::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(_device->GetLogger(), vkSetDebugUtilsObjectNameEXT(_device->GetHandle(), &info));
    }

    auto CSwapchain::AcquireNextImage(const CBinarySemaphore& semaphore) noexcept -> const CImage& {
        RETINA_PROFILE_SCOPED();
        const auto result =
            vkAcquireNextImageKHR(
                _device->GetHandle(),
                _handle,
                -1_u64,
                semaphore.GetHandle(),
                {},
                &_currentImageIndex
            );
        if (result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_ERROR_SURFACE_LOST_KHR ||
            result == VK_SUBOPTIMAL_KHR
        ) {
            _isLost = true;
        }
        return *_images[_currentImageIndex];
    }

    auto CSwapchain::GetCurrentImage() const noexcept -> const CImage& {
        RETINA_PROFILE_SCOPED();
        return *_images[_currentImageIndex];
    }
}
