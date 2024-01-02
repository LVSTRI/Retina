#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/ImageView.hpp>
#include <Retina/Graphics/Queue.hpp>
#include <Retina/Graphics/Swapchain.hpp>

#include <volk.h>

#include <algorithm>

namespace Retina {
    RETINA_NODISCARD static auto GetImageSparseMemoryRequirements(const CDevice& device, VkImage image) noexcept -> std::vector<VkSparseImageMemoryRequirements> {
        RETINA_PROFILE_SCOPED();
        auto sparseMemoryRequirementCount = 0_u32;
        vkGetImageSparseMemoryRequirements(device.GetHandle(), image, &sparseMemoryRequirementCount, nullptr);
        auto sparseMemoryRequirements = std::vector<VkSparseImageMemoryRequirements>(sparseMemoryRequirementCount);
        vkGetImageSparseMemoryRequirements(device.GetHandle(), image, &sparseMemoryRequirementCount, sparseMemoryRequirements.data());
        return sparseMemoryRequirements;
    }

    RETINA_NODISCARD static auto GetSwapchainImages(const CDevice& device, const CSwapchain& swapchain) noexcept -> std::vector<VkImage> {
        RETINA_PROFILE_SCOPED();
        auto swapchainImageCount = 0_u32;
        vkGetSwapchainImagesKHR(device.GetHandle(), swapchain.GetHandle(), &swapchainImageCount, nullptr);
        auto swapchainImages = std::vector<VkImage>(swapchainImageCount);
        vkGetSwapchainImagesKHR(device.GetHandle(), swapchain.GetHandle(), &swapchainImageCount, swapchainImages.data());
        return swapchainImages;
    }

    CImage::~CImage() noexcept {
        RETINA_PROFILE_SCOPED();
        _imageView.Reset();

        RETINA_LOG_INFO(_device->GetLogger(), "Destroying Image \"{}\"", GetDebugName());
        const auto isSparse = IsFlagEnabled(_createInfo.Flags, EImageCreateFlag::E_SPARSE_BINDING);
        if (_allocation) {
            vmaDestroyImage(_device->GetAllocator(), _handle, _allocation);
        } else if (isSparse) {
            vkDestroyImage(_device->GetHandle(), _handle, nullptr);
        }
    }

    auto CImage::Make(const CDevice& device, const SImageCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto image = CArcPtr(new Self());
        const auto& queueInfo = [&] {
            switch (createInfo.QueueDomain) {
                case EQueueDomain::E_GRAPHICS:
                    return device.GetGraphicsQueue().GetFamilyInfo();
                case EQueueDomain::E_COMPUTE:
                    return device.GetComputeQueue().GetFamilyInfo();
                case EQueueDomain::E_TRANSFER:
                    return device.GetTransferQueue().GetFamilyInfo();
            }
            std::unreachable();
        }();
        RETINA_LOG_INFO(
            device.GetLogger(),
            "Creating Image \"{}\" - {}x{}x{}",
            createInfo.Name,
            createInfo.Width,
            createInfo.Height,
            createInfo.Layers
        );
        RETINA_LOG_INFO(device.GetLogger(), "- Flags: {}", ToString(createInfo.Flags));
        RETINA_LOG_INFO(device.GetLogger(), "- Usage: {}", ToString(createInfo.Usage));
        RETINA_LOG_INFO(device.GetLogger(), "- Format: {}", ToString(createInfo.Format));

        const auto sharingMode = createInfo.IsCrossDomain
            ? VK_SHARING_MODE_CONCURRENT
            : VK_SHARING_MODE_EXCLUSIVE;
        const auto queueFamilyIndices = [&] -> std::vector<uint32> {
            if (createInfo.IsCrossDomain) {
                auto families = std::vector {
                    device.GetGraphicsQueue().GetFamilyIndex(),
                    device.GetComputeQueue().GetFamilyIndex(),
                    device.GetTransferQueue().GetFamilyIndex()
                };
                std::sort(families.begin(), families.end());
                families.erase(std::unique(families.begin(), families.end()), families.end());
                return families;
            } else {
                return { queueInfo.FamilyIndex };
            }
        }();

        const auto aspectMask = ImageAspectMaskFromFormat(createInfo.Format);
        auto imageCreateInfo = VkImageCreateInfo(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
        imageCreateInfo.flags = ToEnumCounterpart(createInfo.Flags);
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = ToEnumCounterpart(createInfo.Format);
        imageCreateInfo.extent = VkExtent3D(createInfo.Width, createInfo.Height, 1);
        imageCreateInfo.mipLevels = createInfo.Levels;
        imageCreateInfo.arrayLayers = createInfo.Layers;
        imageCreateInfo.samples = ToEnumCounterpart(createInfo.Samples);
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = ToEnumCounterpart(createInfo.Usage);
        imageCreateInfo.sharingMode = sharingMode;
        imageCreateInfo.queueFamilyIndexCount = queueFamilyIndices.size();
        imageCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        auto imageHandle = VkImage();
        auto memoryRequirements = VkMemoryRequirements();
        auto sparseMemoryRequirement = VkSparseImageMemoryRequirements();
        auto allocationHandle = VmaAllocation();
        auto allocationInfo = VmaAllocationInfo();
        if ((createInfo.Flags & EImageCreateFlag::E_SPARSE_BINDING) == EImageCreateFlag::E_SPARSE_BINDING) {
            RETINA_VULKAN_CHECK(
                device.GetLogger(),
                vkCreateImage(
                    device.GetHandle(),
                    &imageCreateInfo,
                    nullptr,
                    &imageHandle
                )
            );
            vkGetImageMemoryRequirements(device.GetHandle(), imageHandle, &memoryRequirements);
            auto sparseMemoryRequirements = GetImageSparseMemoryRequirements(device, imageHandle);
            const auto nativeAspectMask = ToEnumCounterpart(aspectMask);
            for (const auto& requirement : sparseMemoryRequirements) {
                if (requirement.formatProperties.aspectMask & nativeAspectMask) {
                    sparseMemoryRequirement = requirement;
                    break;
                }
            }
            RETINA_ASSERT_WITH(sparseMemoryRequirement.imageMipTailStride > 0, "Failed to find appropriate Sparse Memory Info");
        } else {
            auto allocationCreateInfo = VmaAllocationCreateInfo();
            allocationCreateInfo.flags = {};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            allocationCreateInfo.requiredFlags = {};
            allocationCreateInfo.preferredFlags = {};
            allocationCreateInfo.memoryTypeBits = {};
            allocationCreateInfo.pool = nullptr;
            allocationCreateInfo.pUserData = nullptr;
            allocationCreateInfo.priority = 1.0f;

            RETINA_VULKAN_CHECK(
                device.GetLogger(),
                vmaCreateImage(
                    device.GetAllocator(),
                    &imageCreateInfo,
                    &allocationCreateInfo,
                    &imageHandle,
                    &allocationHandle,
                    &allocationInfo
                )
            );
            vkGetImageMemoryRequirements(device.GetHandle(), imageHandle, &memoryRequirements);
        }

        image->_handle = imageHandle;
        image->_allocation = allocationHandle;
        image->_allocationInfo = allocationInfo;
        image->_createInfo = createInfo;
        image->_device = device.ToArcPtr();
        image->SetDebugName(createInfo.Name);
        if (createInfo.ViewInfo) {
            auto viewInfo = *createInfo.ViewInfo;
            if (viewInfo.Name.empty()) {
                viewInfo.Name = std::format("{}View", createInfo.Name);
            }
            image->_imageView = CImageView::Make(*image, viewInfo).AsConst();
        }

        return image;
    }

    auto CImage::FromSwapchain(
        const CDevice& device,
        const CSwapchain& swapchain,
        const SImageCreateInfo& createInfo
    ) noexcept -> std::vector<CArcPtr<Self>> {
        RETINA_PROFILE_SCOPED();
        auto images = std::vector<CArcPtr<Self>>();
        const auto imageHandles = GetSwapchainImages(device, swapchain);
        for (auto imageIndex = 0_u32; const auto& imageHandle : imageHandles) {
            auto memoryRequirements = VkMemoryRequirements();
            vkGetImageMemoryRequirements(device.GetHandle(), imageHandle, &memoryRequirements);

            const auto imageName = std::format("{}{}", createInfo.Name, imageIndex++);
            auto image = CArcPtr(new Self());
            image->_handle = imageHandle;
            image->_memoryRequirements = memoryRequirements;
            image->_createInfo = createInfo;
            image->_device = device.ToArcPtr();
            image->SetDebugName(imageName);
            if (createInfo.ViewInfo) {
                auto viewInfo = *createInfo.ViewInfo;
                if (viewInfo.Name.empty()) {
                    viewInfo.Name = std::format("{}View", imageName);
                }
                image->_imageView = CImageView::Make(*image, viewInfo).AsConst();
            }

            images.emplace_back(std::move(image));
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

    auto CImage::GetView() const noexcept -> const CImageView& {
        RETINA_PROFILE_SCOPED();
        return *_imageView;
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

    auto CImage::GetLevels() const noexcept -> uint32 {
        RETINA_PROFILE_SCOPED();
        return _createInfo.Levels;
    }

    auto CImage::GetLayers() const noexcept -> uint32 {
        RETINA_PROFILE_SCOPED();
        return _createInfo.Layers;
    }

    auto CImage::GetQueueDomain() const noexcept -> EQueueDomain {
        RETINA_PROFILE_SCOPED();
        return _createInfo.QueueDomain;
    }

    auto CImage::GetSamples() const noexcept -> ESampleCount {
        RETINA_PROFILE_SCOPED();
        return _createInfo.Samples;
    }

    auto CImage::GetUsage() const noexcept -> EImageUsage {
        RETINA_PROFILE_SCOPED();
        return _createInfo.Usage;
    }

    auto CImage::GetFormat() const noexcept -> EResourceFormat {
        RETINA_PROFILE_SCOPED();
        return _createInfo.Format;
    }

    auto CImage::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_IMAGE;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(_device->GetLogger(), vkSetDebugUtilsObjectNameEXT(_device->GetHandle(), &info));
    }

    auto CImage::GetDescriptor(EImageLayout layout) const noexcept -> SImageDescriptor {
        RETINA_PROFILE_SCOPED();
        return SImageDescriptor {
            .View = _imageView->GetHandle(),
            .Layout = layout
        };
    }
}
