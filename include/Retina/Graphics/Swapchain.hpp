#pragma once

#include <Retina/Graphics/SwapchainInfo.hpp>

namespace Retina::Graphics {
  class CSwapchain : public Core::IEnableIntrusiveReferenceCount<CSwapchain> {
  public:
    CSwapchain(const WSI::CWindow& window) noexcept;
    ~CSwapchain() noexcept;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      const WSI::CWindow& window,
      const SSwapchainCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CSwapchain>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkSwapchainKHR;
    RETINA_NODISCARD auto GetSurface() const noexcept -> VkSurfaceKHR;

    RETINA_NODISCARD auto GetWidth() const noexcept -> uint32;
    RETINA_NODISCARD auto GetHeight() const noexcept -> uint32;
    RETINA_NODISCARD auto GetFormat() const noexcept -> EResourceFormat;
    RETINA_NODISCARD auto GetImages() const noexcept -> std::span<const Core::CArcPtr<CImage>>;

    RETINA_NODISCARD auto GetCurrentImageIndex() const noexcept -> uint32;
    RETINA_NODISCARD auto IsValid() const noexcept -> bool;

    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SSwapchainCreateInfo&;
    RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;
    RETINA_NODISCARD auto GetWindow() const noexcept -> const WSI::CWindow&;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

    RETINA_NODISCARD auto IsVSyncEnabled() const noexcept -> bool;

    auto AcquireNextImage(const CBinarySemaphore& semaphore) noexcept -> bool;
    auto Present(const SSwapchainPresentInfo& presentInfo) noexcept -> bool;

    RETINA_NODISCARD auto GetCurrentImage() const noexcept -> const CImage&;

  private:
    VkSwapchainKHR _handle = {};
    VkSurfaceKHR _surface = {};

    uint32 _width = 0;
    uint32 _height = 0;
    EResourceFormat _format = {};
    std::vector<Core::CArcPtr<CImage>> _images;

    uint32 _currentImageIndex = 0;
    bool _isValid = false;

    SSwapchainCreateInfo _createInfo = {};
    Core::CArcPtr<const CDevice> _device;
    std::reference_wrapper<const WSI::CWindow> _window;
  };
}