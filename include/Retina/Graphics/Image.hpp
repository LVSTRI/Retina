#pragma once

#include <Retina/Graphics/Forward.hpp>
#include <Retina/Graphics/ImageInfo.hpp>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Retina::Graphics {
  class CImage : public Core::IEnableIntrusiveReferenceCount<CImage> {
  public:
    CImage(const CDevice& device) noexcept;
    ~CImage() noexcept;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      const SImageCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CImage>;

    RETINA_NODISCARD static auto FromSwapchain(
      const CDevice& device,
      const CSwapchain& swapchain,
      const SImageCreateInfo& info
    ) noexcept -> std::vector<Core::CArcPtr<CImage>>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkImage;
    RETINA_NODISCARD auto GetAllocation() const noexcept -> VmaAllocation;
    RETINA_NODISCARD auto GetAllocationInfo() const noexcept -> const VmaAllocationInfo&;

    RETINA_NODISCARD auto GetMemoryRequirements() const noexcept -> const VkMemoryRequirements&;
    RETINA_NODISCARD auto GetSparseMemoryRequirements() const noexcept -> const VkSparseImageMemoryRequirements&;

    RETINA_NODISCARD auto GetView() const noexcept -> const CImageView&;
    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SImageCreateInfo&;
    RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

    RETINA_NODISCARD auto GetWidth() const noexcept -> uint32;
    RETINA_NODISCARD auto GetHeight() const noexcept -> uint32;
    RETINA_NODISCARD auto GetLevelCount() const noexcept -> uint32;
    RETINA_NODISCARD auto GetLayerCount() const noexcept -> uint32;
    RETINA_NODISCARD auto GetFormat() const noexcept -> EResourceFormat;
    RETINA_NODISCARD auto GetUsage() const noexcept -> EImageUsageFlag;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

  private:
    VkImage _handle = {};
    VmaAllocation _allocation = {};
    VmaAllocationInfo _allocationInfo = {};

    VkMemoryRequirements _memoryRequirements = {};
    VkSparseImageMemoryRequirements _sparseMemoryRequirements = {};

    Core::CArcPtr<CImageView> _view;

    SImageCreateInfo _createInfo = {};
    Core::CReferenceWrapper<const CDevice> _device;
  };
}
