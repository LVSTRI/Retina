#include <Retina/Graphics/BinarySemaphore.hpp>
#include <Retina/Graphics/DeletionQueue.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Instance.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Queue.hpp>
#include <Retina/Graphics/Macros.hpp>
#include <Retina/Graphics/Swapchain.hpp>

#include <Retina/WSI/Window.hpp>

#include <volk.h>

#include <algorithm>
#include <ranges>
#include <array>

namespace Retina::Graphics {
  namespace Details {
    RETINA_NODISCARD RETINA_INLINE auto IsSurfaceSupported(
      const CDevice& device,
      EQueueDomain domain,
      VkSurfaceKHR surface
    ) noexcept -> bool {
      auto isSupported = VkBool32();
      const auto queueFamily = [&] {
        switch (domain) {
          case EQueueDomain::E_GRAPHICS:
            return device.GetGraphicsQueue().GetFamilyIndex();
          case EQueueDomain::E_COMPUTE:
            return device.GetComputeQueue().GetFamilyIndex();
          case EQueueDomain::E_TRANSFER:
            return device.GetTransferQueue().GetFamilyIndex();
          default:
            std::unreachable();
        }
      }();
      RETINA_GRAPHICS_VULKAN_CHECK(
        vkGetPhysicalDeviceSurfaceSupportKHR(
          device.GetPhysicalDevice(),
          queueFamily,
          surface,
          &isSupported
        )
      );
      return isSupported;
    }

    RETINA_NODISCARD RETINA_INLINE auto GetSurfaceCapabilities(
      const CDevice& device,
      VkSurfaceKHR surface
    ) noexcept -> VkSurfaceCapabilitiesKHR {
      auto capabilities = VkSurfaceCapabilitiesKHR();
      RETINA_GRAPHICS_VULKAN_CHECK(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
          device.GetPhysicalDevice(),
          surface,
          &capabilities
        )
      );
      return capabilities;
    }

    RETINA_NODISCARD RETINA_INLINE auto GetSurfaceFormats(
      const CDevice& device,
      VkSurfaceKHR surface
    ) noexcept -> std::vector<VkSurfaceFormatKHR> {
      auto count = 0_u32;
      RETINA_GRAPHICS_VULKAN_CHECK(
        vkGetPhysicalDeviceSurfaceFormatsKHR(
          device.GetPhysicalDevice(),
          surface,
          &count,
          nullptr
        )
      );
      auto formats = std::vector<VkSurfaceFormatKHR>(count);
      RETINA_GRAPHICS_VULKAN_CHECK(
        vkGetPhysicalDeviceSurfaceFormatsKHR(
          device.GetPhysicalDevice(),
          surface,
          &count,
          formats.data()
        )
      );
      for (const auto format : formats) {
        RETINA_GRAPHICS_INFO(
          "Supported surface format: {{ Format: {}, Color Space: {} }}",
          ToString(format.format),
          ToString(format.colorSpace)
        );
      }
      return formats;
    }

RETINA_NODISCARD RETINA_INLINE auto GetSurfacePresentModes(
      const CDevice& device,
      VkSurfaceKHR surface
    ) noexcept -> std::vector<VkPresentModeKHR> {
      auto count = 0_u32;
      RETINA_GRAPHICS_VULKAN_CHECK(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
          device.GetPhysicalDevice(),
          surface,
          &count,
          nullptr
        )
      );
      auto presentModes = std::vector<VkPresentModeKHR>(count);
      RETINA_GRAPHICS_VULKAN_CHECK(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
          device.GetPhysicalDevice(),
          surface,
          &count,
          presentModes.data()
        )
      );
      for (const auto presentMode : presentModes) {
        RETINA_GRAPHICS_INFO("Supported present mode: {}", ToString(presentMode));
      }
      return presentModes;
    }

    RETINA_NODISCARD RETINA_INLINE auto SelectSwapchainImageCount(
      const VkSurfaceCapabilitiesKHR& capabilities
    ) noexcept -> uint32 {
      constexpr static auto targetImageCount = 3_u32;
      const auto minImageCount = capabilities.minImageCount ? capabilities.minImageCount : targetImageCount;
      const auto maxImageCount = capabilities.maxImageCount ? capabilities.maxImageCount : targetImageCount;
      const auto imageCount = std::clamp(targetImageCount, minImageCount, maxImageCount);
      RETINA_GRAPHICS_INFO(" - Selected image count: {}", imageCount);
      return imageCount;
    }

    RETINA_NODISCARD RETINA_INLINE auto SelectSurfaceColorSpace(
      std::span<const VkSurfaceFormatKHR> formats
    ) noexcept -> VkColorSpaceKHR {
      for (const auto& format : formats) {
        if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
          RETINA_GRAPHICS_INFO(" - Selected color space: {}", ToString(format.colorSpace));
          return format.colorSpace;
        }
      }
      return formats[0].colorSpace;
    }

    RETINA_NODISCARD RETINA_INLINE auto SelectSurfaceFormat(
      std::span<const VkSurfaceFormatKHR> formats
    ) noexcept -> VkFormat {
      const auto targetFormats = std::to_array({
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_UNORM,
      });
      for (const auto& targetFormat : targetFormats) {
        const auto isAvailable = std::ranges::any_of(formats, [&](const auto& format) {
          return format.format == targetFormat;
        });
        if (isAvailable) {
          RETINA_GRAPHICS_INFO(" - Selected format: {}", ToString(targetFormat));
          return targetFormat;
        }
      }
      return formats[0].format;
    }

    RETINA_NODISCARD RETINA_INLINE auto SelectSurfacePresentMode(
      const SSwapchainCreateInfo& createInfo,
      std::span<const VkPresentModeKHR> presentModes
    ) noexcept -> VkPresentModeKHR {
      const auto targetPresentMode = createInfo.VSync
        ? VK_PRESENT_MODE_FIFO_KHR
        : VK_PRESENT_MODE_MAILBOX_KHR;
      for (const auto presentMode : presentModes) {
        if (presentMode == targetPresentMode) {
          RETINA_GRAPHICS_INFO(" - Selected present mode: {}", ToString(presentMode));
          return presentMode;
        }
      }
      return VK_PRESENT_MODE_FIFO_KHR;
    }

    RETINA_NODISCARD RETINA_INLINE auto SelectSurfaceExtent(
      const WSI::CWindow& window,
      const VkSurfaceCapabilitiesKHR& capabilities
    ) noexcept -> VkExtent2D {
      const auto& currentExtent = capabilities.currentExtent;
      const auto& minImageExtent = capabilities.minImageExtent;
      const auto& maxImageExtent = capabilities.maxImageExtent;
      if (currentExtent.width != -1_u32 && currentExtent.height != -1_u32) {
        RETINA_GRAPHICS_INFO(" - Selected extent: {{ Width: {}, Height: {} }}", currentExtent.width, currentExtent.height);
        return currentExtent;
      }
      const auto minExtent = VkExtent2D(
        minImageExtent.width == -1_u32 ? 0 : minImageExtent.width,
        minImageExtent.height == -1_u32 ? 0 : minImageExtent.height
      );
      const auto width = std::clamp(window.GetWidth(), minExtent.width, maxImageExtent.width);
      const auto height = std::clamp(window.GetHeight(), minExtent.height, maxImageExtent.height);
      RETINA_GRAPHICS_INFO(" - Selected extent: {{ Width: {}, Height: {} }}", width, height);
      return VkExtent2D(width, height);
    }
  }

  CSwapchain::CSwapchain(const WSI::CWindow& window) noexcept
    : _window(window)
  {
    RETINA_PROFILE_SCOPED();
  }

  CSwapchain::~CSwapchain() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      _images.clear();
      vkDestroySwapchainKHR(_device->GetHandle(), _handle, nullptr);
      RETINA_GRAPHICS_INFO("Swapchain ({}) destroyed", GetDebugName());
    }
    if (_surface) {
      vkDestroySurfaceKHR(_device->GetInstance().GetHandle(), _surface, nullptr);
    }
  }

  auto CSwapchain::Make(
    const CDevice& device,
    const WSI::CWindow& window,
    const SSwapchainCreateInfo& createInfo,
    CSwapchain* oldSwapchain
  ) noexcept -> Core::CArcPtr<CSwapchain> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CSwapchain(window));
    auto surface = VkSurfaceKHR();
    if (oldSwapchain) {
      surface = oldSwapchain->_surface;
    } else {
      surface = static_cast<VkSurfaceKHR>(createInfo.MakeSurface(device.GetInstance().GetHandle(), window.GetHandle()));
      if (!surface) {
        RETINA_GRAPHICS_PANIC_WITH("Failed to create surface for swapchain: {}", createInfo.Name);
      }
    }
    if (!Details::IsSurfaceSupported(device, EQueueDomain::E_GRAPHICS, surface)) {
      RETINA_GRAPHICS_PANIC_WITH("Surface not supported by graphics queue for swapchain: {}", createInfo.Name);
    }

    const auto surfaceCapabilities = Details::GetSurfaceCapabilities(device, surface);
    const auto surfaceFormats = Details::GetSurfaceFormats(device, surface);
    const auto surfacePresentModes = Details::GetSurfacePresentModes(device, surface);
    const auto swapchainImageCount = Details::SelectSwapchainImageCount(surfaceCapabilities);
    const auto surfaceColorSpace = Details::SelectSurfaceColorSpace(surfaceFormats);
    const auto surfaceFormat = Details::SelectSurfaceFormat(surfaceFormats);
    const auto surfacePresentMode = Details::SelectSurfacePresentMode(createInfo, surfacePresentModes);
    const auto surfaceExtent = Details::SelectSurfaceExtent(window, surfaceCapabilities);
    const auto queueFamily = device.GetGraphicsQueue().GetFamilyIndex();

    auto swapchainCreateInfo = VkSwapchainCreateInfoKHR(VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = swapchainImageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat;
    swapchainCreateInfo.imageColorSpace = surfaceColorSpace;
    swapchainCreateInfo.imageExtent = surfaceExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
      VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 1;
    swapchainCreateInfo.pQueueFamilyIndices = &queueFamily;
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = surfacePresentMode;
    swapchainCreateInfo.clipped = true;
    if (oldSwapchain) {
      swapchainCreateInfo.oldSwapchain = oldSwapchain->_handle;
    }

    auto swapchainHandle = VkSwapchainKHR();
    RETINA_GRAPHICS_VULKAN_CHECK(vkCreateSwapchainKHR(device.GetHandle(), &swapchainCreateInfo, nullptr, &swapchainHandle));
    RETINA_GRAPHICS_INFO("Swapchain ({}) initialized", createInfo.Name);
    self->_handle = swapchainHandle;
    self->_surface = surface;
    self->_width = surfaceExtent.width;
    self->_height = surfaceExtent.height;
    self->_format = AsEnumCounterpart(surfaceFormat);
    self->_isValid = true;
    self->_createInfo = createInfo;
    self->_device = device.ToArcPtr();
    self->SetDebugName(createInfo.Name);

    auto swapchainImages = CImage::FromSwapchain(device, *self, {
      .Name = "SwapchainImage",
      .Width = self->_width,
      .Height = self->_height,
      .Format = self->_format,
      .Usage =
        EImageUsageFlag::E_COLOR_ATTACHMENT |
        EImageUsageFlag::E_TRANSFER_DST,
      .ViewInfo = DEFAULT_IMAGE_VIEW_CREATE_INFO,
    });
    self->_images = std::move(swapchainImages);

    return self;
  }

  auto CSwapchain::Recreate(Core::CArcPtr<CSwapchain>&& oldSwapchain) noexcept -> Core::CArcPtr<CSwapchain> {
    RETINA_PROFILE_SCOPED();
    const auto& device = oldSwapchain->GetDevice();
    const auto& window = oldSwapchain->GetWindow();
    const auto createInfo = oldSwapchain->GetCreateInfo();
    auto self = Make(device, window, createInfo, oldSwapchain.Get());
    device.GetDeletionQueue().Enqueue([oldSwapchain = std::move(oldSwapchain)] mutable noexcept {
      // Don't delete the surface, we still need it
      oldSwapchain->_surface = {};
    });
    return self;
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

  auto CSwapchain::GetImages() const noexcept -> std::span<const Core::CArcPtr<CImage>> {
    RETINA_PROFILE_SCOPED();
    return _images;
  }

  auto CSwapchain::GetCurrentImageIndex() const noexcept -> uint32 {
    RETINA_PROFILE_SCOPED();
    return _currentImageIndex;
  }

  auto CSwapchain::IsValid() const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return _isValid;
  }

  auto CSwapchain::GetCreateInfo() const noexcept -> const SSwapchainCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CSwapchain::GetDevice() const noexcept -> const CDevice& {
    RETINA_PROFILE_SCOPED();
    return *_device;
  }

  auto CSwapchain::GetWindow() const noexcept -> const WSI::CWindow& {
    RETINA_PROFILE_SCOPED();
    return _window;
  }

  auto CSwapchain::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CSwapchain::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_SET_DEBUG_NAME(GetDevice().GetHandle(), _handle, VK_OBJECT_TYPE_SWAPCHAIN_KHR, name);
    _createInfo.Name = name;
  }

  auto CSwapchain::IsVSyncEnabled() const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return _createInfo.VSync;
  }

  auto CSwapchain::AcquireNextImage(const CBinarySemaphore& semaphore) noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    if (!_isValid) {
      return false;
    }
    const auto result =
      vkAcquireNextImageKHR(
        _device->GetHandle(),
        _handle,
        -1_u64,
        semaphore.GetHandle(),
        {},
        &_currentImageIndex
      );
    if (
      result == VK_ERROR_OUT_OF_DATE_KHR ||
      result == VK_ERROR_SURFACE_LOST_KHR ||
      result == VK_SUBOPTIMAL_KHR
    ) {
      RETINA_GRAPHICS_WARN("Swapchain ({}) out of date", GetDebugName());
      _isValid = false;
      return false;
    }
    RETINA_GRAPHICS_VULKAN_CHECK(result);
    return true;
  }

  auto CSwapchain::Present(const SSwapchainPresentInfo& presentInfo) noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    if (!_isValid) {
      return false;
    }
    auto& queue = _device->GetGraphicsQueue();
    auto waitSemaphores = std::vector<VkSemaphore>();
    waitSemaphores.reserve(presentInfo.WaitSemaphores.size());
    for (const auto& semaphore : presentInfo.WaitSemaphores) {
      waitSemaphores.emplace_back(semaphore->GetHandle());
    }

    auto queuePresentInfo = VkPresentInfoKHR(VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
    queuePresentInfo.waitSemaphoreCount = waitSemaphores.size();
    queuePresentInfo.pWaitSemaphores = waitSemaphores.data();
    queuePresentInfo.swapchainCount = 1;
    queuePresentInfo.pSwapchains = &_handle;
    queuePresentInfo.pImageIndices = &_currentImageIndex;
    queuePresentInfo.pResults = nullptr;

    queue.Lock();
    const auto result = vkQueuePresentKHR(queue.GetHandle(), &queuePresentInfo);
    queue.Unlock();

    if (
      result == VK_ERROR_OUT_OF_DATE_KHR ||
      result == VK_ERROR_SURFACE_LOST_KHR ||
      result == VK_SUBOPTIMAL_KHR
    ) {
      RETINA_GRAPHICS_WARN("Swapchain ({}) out of date", GetDebugName());
      _isValid = false;
      return false;
    }
    RETINA_GRAPHICS_VULKAN_CHECK(result);
    return true;
  }

  auto CSwapchain::GetCurrentImage() const noexcept -> const CImage& {
    RETINA_PROFILE_SCOPED();
    return *_images[_currentImageIndex];
  }
}
