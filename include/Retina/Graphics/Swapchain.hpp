#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/SwapchainInfo.hpp>

#include <vulkan/vulkan.h>

#include <vector>
#include <span>

namespace Retina {
    class CSwapchain : public INativeDebugName, public IEnableIntrusiveReferenceCount<CSwapchain> {
    public:
        using Self = CSwapchain;

        CSwapchain() noexcept = default;
        ~CSwapchain() noexcept;

        RETINA_NODISCARD static auto Make(const CDevice& device, const CWindow& window, const SSwapchainCreateInfo& createInfo) noexcept -> CArcPtr<Self>;
        RETINA_NODISCARD static auto Recreate(
            CArcPtr<Self>&& oldSwapchain,
            const SSwapchainCreateInfo* createInfo = nullptr
        ) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkSwapchainKHR;
        RETINA_NODISCARD auto GetSurface() const noexcept -> VkSurfaceKHR;
        RETINA_NODISCARD auto GetWidth() const noexcept -> uint32;
        RETINA_NODISCARD auto GetHeight() const noexcept -> uint32;
        RETINA_NODISCARD auto GetFormat() const noexcept -> EResourceFormat;
        RETINA_NODISCARD auto GetImages() const noexcept -> std::span<const CArcPtr<CImage>>;
        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SSwapchainCreateInfo&;
        RETINA_NODISCARD auto GetCurrentImageIndex() const noexcept -> uint32;
        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;
        RETINA_NODISCARD auto GetWindow() const noexcept -> const CWindow&;

        auto SetDebugName(std::string_view name) noexcept -> void;

        RETINA_NODISCARD auto AcquireNextImage(const CBinarySemaphore& semaphore) noexcept -> bool;
        RETINA_NODISCARD auto GetCurrentImage() const noexcept -> const CImage&;

    private:
        VkSwapchainKHR _handle = {};
        VkSurfaceKHR _surface = {};

        uint32 _width = 0;
        uint32 _height = 0;
        EResourceFormat _format = {};
        std::vector<CArcPtr<CImage>> _images;

        uint32 _currentImageIndex = 0;

        SSwapchainCreateInfo _createInfo = {};
        CArcPtr<const CDevice> _device;
        CArcPtr<const CWindow> _window;
    };
}

