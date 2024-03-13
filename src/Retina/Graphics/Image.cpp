#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/ImageView.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>
#include <Retina/Graphics/Queue.hpp>
#include <Retina/Graphics/Swapchain.hpp>

#include <volk.h>

#include <algorithm>

namespace Retina::Graphics {
  namespace Details {
    RETINA_NODISCARD RETINA_INLINE auto GetImageMemoryRequirements(
      const CDevice& device,
      VkImage image
    ) noexcept -> VkMemoryRequirements {
      RETINA_PROFILE_SCOPED();
      auto requirements = VkMemoryRequirements();
      vkGetImageMemoryRequirements(device.GetHandle(), image, &requirements);
      return requirements;
    }

    RETINA_NODISCARD RETINA_INLINE auto GetImageSparseMemoryRequirements(
      const CDevice& device,
      VkImage image
    ) noexcept -> std::vector<VkSparseImageMemoryRequirements> {
      RETINA_PROFILE_SCOPED();
      auto count = 0_u32;
      vkGetImageSparseMemoryRequirements(device.GetHandle(), image, &count, nullptr);
      auto requirements = std::vector<VkSparseImageMemoryRequirements>(count);
      vkGetImageSparseMemoryRequirements(device.GetHandle(), image, &count, requirements.data());
      return requirements;
    }

    RETINA_NODISCARD RETINA_INLINE auto GetSwapchainImages(
      const CDevice& device,
      const CSwapchain& swapchain
    ) noexcept -> std::vector<VkImage> {
      RETINA_PROFILE_SCOPED();
      auto count = 0_u32;
      RETINA_GRAPHICS_VULKAN_CHECK(vkGetSwapchainImagesKHR(device.GetHandle(), swapchain.GetHandle(), &count, nullptr));
      auto images = std::vector<VkImage>(count);
      RETINA_GRAPHICS_VULKAN_CHECK(vkGetSwapchainImagesKHR(device.GetHandle(), swapchain.GetHandle(), &count, images.data()));
      return images;
    }
  }

  CImage::CImage(const CDevice& device) noexcept
    : _device(device)
  {
    RETINA_PROFILE_SCOPED();
  }

  CImage::~CImage() noexcept {
    RETINA_PROFILE_SCOPED();
    _view.Reset();
    const auto isSparse = Core::IsFlagEnabled(_createInfo.Flags, EImageCreateFlag::E_SPARSE_BINDING);
    if (_allocation) {
      vmaDestroyImage(_device->GetAllocator(), _handle, _allocation);
      RETINA_GRAPHICS_INFO("Image ({}) destroyed", GetDebugName());
    } else if (_handle && isSparse) {
      vkDestroyImage(_device->GetHandle(), _handle, nullptr);
      RETINA_GRAPHICS_INFO("Image ({}) destroyed", GetDebugName());
    }
  }

  auto CImage::Make(const CDevice& device, const SImageCreateInfo& createInfo) noexcept -> Core::CArcPtr<CImage> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CImage(device));
    const auto sharingMode = createInfo.IsCrossDomain
      ? VK_SHARING_MODE_CONCURRENT
      : VK_SHARING_MODE_EXCLUSIVE;
    const auto queueFamilies = [&] noexcept -> std::vector<uint32> {
      if (createInfo.IsCrossDomain) {
        auto families = std::vector<uint32> {
          device.GetGraphicsQueue().GetFamilyIndex(),
          device.GetComputeQueue().GetFamilyIndex(),
          device.GetTransferQueue().GetFamilyIndex()
        };
        std::sort(families.begin(), families.end());
        families.erase(std::unique(families.begin(), families.end()), families.end());
        return families;
      }
      switch (createInfo.Domain) {
        case EQueueDomain::E_GRAPHICS:
          return { device.GetGraphicsQueue().GetFamilyIndex() };
        case EQueueDomain::E_COMPUTE:
          return { device.GetComputeQueue().GetFamilyIndex() };
        case EQueueDomain::E_TRANSFER:
          return { device.GetTransferQueue().GetFamilyIndex() };
      }
      std::unreachable();
    }();

    const auto aspectMask = ImageAspectMaskFrom(createInfo.Format);
    auto imageCreateInfo = VkImageCreateInfo(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
    imageCreateInfo.flags = AsEnumCounterpart(createInfo.Flags);
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = AsEnumCounterpart(createInfo.Format);
    imageCreateInfo.extent = {
      .width = createInfo.Width,
      .height = createInfo.Height,
      .depth = 1
    };
    imageCreateInfo.mipLevels = createInfo.Levels;
    imageCreateInfo.arrayLayers = createInfo.Layers;
    imageCreateInfo.samples = AsEnumCounterpart(createInfo.Samples);
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = AsEnumCounterpart(createInfo.Usage);
    imageCreateInfo.sharingMode = sharingMode;
    imageCreateInfo.queueFamilyIndexCount = queueFamilies.size();
    imageCreateInfo.pQueueFamilyIndices = queueFamilies.data();
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    const auto isAttachment =
      Core::IsFlagEnabled(createInfo.Usage, EImageUsageFlag::E_COLOR_ATTACHMENT) ||
      Core::IsFlagEnabled(createInfo.Usage, EImageUsageFlag::E_DEPTH_STENCIL_ATTACHMENT) ||
      Core::IsFlagEnabled(createInfo.Usage, EImageUsageFlag::E_TRANSIENT_ATTACHMENT) ||
      Core::IsFlagEnabled(createInfo.Usage, EImageUsageFlag::E_INPUT_ATTACHMENT);

    auto imageHandle = VkImage();
    auto allocationHandle = VmaAllocation();
    auto allocationInfo = VmaAllocationInfo();
    auto memoryRequirements = VkMemoryRequirements();
    auto sparseMemoryRequirement = VkSparseImageMemoryRequirements();
    if (Core::IsFlagEnabled(createInfo.Flags, EImageCreateFlag::E_SPARSE_BINDING)) {
      RETINA_GRAPHICS_VULKAN_CHECK(vkCreateImage(device.GetHandle(), &imageCreateInfo, nullptr, &imageHandle));
      memoryRequirements = Details::GetImageMemoryRequirements(device, imageHandle);
      const auto sparseMemoryRequirements = Details::GetImageSparseMemoryRequirements(device, imageHandle);
      const auto nativeAspectMask = AsEnumCounterpart(aspectMask);
      for (const auto& requirement : sparseMemoryRequirements) {
        if (requirement.formatProperties.aspectMask & nativeAspectMask) {
          sparseMemoryRequirement = requirement;
          break;
        }
      }
      RETINA_ASSERT_WITH(sparseMemoryRequirement.imageMipTailStride > 0, "Image mip tail stride must be greater than 0");
    } else {
      auto allocationCreateInfo = VmaAllocationCreateInfo();
      allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
      allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
      allocationCreateInfo.priority = 1.0f;

      if (isAttachment || createInfo.IsDedicated) {
        allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
      }

      vmaCreateImage(
        device.GetAllocator(),
        &imageCreateInfo,
        &allocationCreateInfo,
        &imageHandle,
        &allocationHandle,
        &allocationInfo
      );
      memoryRequirements = Details::GetImageMemoryRequirements(device, imageHandle);
    }

    self->_handle = imageHandle;
    self->_allocation = allocationHandle;
    self->_allocationInfo = allocationInfo;
    self->_memoryRequirements = memoryRequirements;
    self->_sparseMemoryRequirements = sparseMemoryRequirement;
    self->_createInfo = createInfo;
    self->SetDebugName(createInfo.Name);

    if (createInfo.ViewInfo) {
      auto viewInfo = *createInfo.ViewInfo;
      if (viewInfo.Name.empty()) {
        viewInfo.Name = std::format("{}View", createInfo.Name);
      }
      self->_view = CImageView::Make(*self, viewInfo);
    }

    RETINA_GRAPHICS_INFO("Image ({}) initialized", createInfo.Name);
    return self;
  }

  auto CImage::FromSwapchain(
    const CDevice& device,
    const CSwapchain& swapchain,
    const SImageCreateInfo& info
  ) noexcept -> std::vector<Core::CArcPtr<CImage>> {
    RETINA_PROFILE_SCOPED();
    auto images = std::vector<Core::CArcPtr<CImage>>();
    const auto swapchainImages = Details::GetSwapchainImages(device, swapchain);
    for (auto i = 0_u32; i < swapchainImages.size(); ++i) {
      const auto swapchainImage = swapchainImages[i];
      auto currentImageCreateInfo = info;
      currentImageCreateInfo.Name += std::to_string(i);

      auto self = Core::CArcPtr(new CImage(device));
      self->_handle = swapchainImage;
      self->SetDebugName(currentImageCreateInfo.Name);

      if (currentImageCreateInfo.ViewInfo) {
        auto viewInfo = *currentImageCreateInfo.ViewInfo;
        if (viewInfo.Name.empty()) {
          viewInfo.Name = std::format("{}View", currentImageCreateInfo.Name);
        }
        self->_createInfo = std::move(currentImageCreateInfo);
        self->_view = CImageView::Make(*self, std::move(viewInfo));
      }
      images.emplace_back(std::move(self));
    }
    return images;
  }

  auto CImage::GetHandle() const noexcept -> VkImage {
    RETINA_PROFILE_SCOPED();
    return _handle;
  }

  auto CImage::GetAllocation() const noexcept -> VmaAllocation {
    RETINA_PROFILE_SCOPED();
    return _allocation;
  }

  auto CImage::GetAllocationInfo() const noexcept -> const VmaAllocationInfo& {
    RETINA_PROFILE_SCOPED();
    return _allocationInfo;
  }

  auto CImage::GetMemoryRequirements() const noexcept -> const VkMemoryRequirements& {
    RETINA_PROFILE_SCOPED();
    return _memoryRequirements;
  }

  auto CImage::GetSparseMemoryRequirements() const noexcept -> const VkSparseImageMemoryRequirements& {
    RETINA_PROFILE_SCOPED();
    return _sparseMemoryRequirements;
  }

  auto CImage::GetView() const noexcept -> const CImageView& {
    RETINA_PROFILE_SCOPED();
    return *_view;
  }

  auto CImage::GetCreateInfo() const noexcept -> const SImageCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CImage::GetDevice() const noexcept -> const CDevice& {
    RETINA_PROFILE_SCOPED();
    return *_device;
  }

  auto CImage::GetWidth() const noexcept -> uint32 {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Width;
  }

  auto CImage::GetHeight() const noexcept -> uint32 {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Height;
  }

  auto CImage::GetLevelCount() const noexcept -> uint32 {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Levels;
  }

  auto CImage::GetLayerCount() const noexcept -> uint32 {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Layers;
  }

  auto CImage::GetFormat() const noexcept -> EResourceFormat {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Format;
  }

  auto CImage::GetUsage() const noexcept -> EImageUsageFlag {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Usage;
  }

  auto CImage::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CImage::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_SET_DEBUG_NAME(_device->GetHandle(), _handle, VK_OBJECT_TYPE_IMAGE, name);
    _createInfo.Name = name;
  }
}
