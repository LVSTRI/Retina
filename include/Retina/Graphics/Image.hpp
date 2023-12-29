#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/DescriptorSetInfo.hpp>
#include <Retina/Graphics/ImageInfo.hpp>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Retina {
    class CImage : public INativeDebugName, public IEnableIntrusiveReferenceCount<CImage> {
    public:
        using Self = CImage;

        CImage() noexcept = default;
        ~CImage() noexcept;

        RETINA_NODISCARD static auto Make(const CDevice& device, const SImageCreateInfo& createInfo) noexcept -> CArcPtr<Self>;
        RETINA_NODISCARD static auto FromSwapchain(
            const CDevice& device,
            const CSwapchain& swapchain,
            const SImageCreateInfo& createInfo
        ) noexcept -> std::vector<CArcPtr<Self>>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkImage;
        RETINA_NODISCARD auto GetAllocation() const noexcept -> VmaAllocation;
        RETINA_NODISCARD auto GetAllocationInfo() const noexcept -> const VmaAllocationInfo&;
        RETINA_NODISCARD auto GetView() const noexcept -> const CImageView&;

        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SImageCreateInfo&;
        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

        RETINA_NODISCARD auto GetWidth() const noexcept -> uint32;
        RETINA_NODISCARD auto GetHeight() const noexcept -> uint32;
        RETINA_NODISCARD auto GetLevels() const noexcept -> uint32;
        RETINA_NODISCARD auto GetLayers() const noexcept -> uint32;
        RETINA_NODISCARD auto GetQueueDomain() const noexcept -> EQueueDomain;
        RETINA_NODISCARD auto GetSamples() const noexcept -> ESampleCount;
        RETINA_NODISCARD auto GetUsage() const noexcept -> EImageUsage;
        RETINA_NODISCARD auto GetFormat() const noexcept -> EResourceFormat;

        auto SetDebugName(std::string_view name) noexcept -> void;

        RETINA_NODISCARD auto GetDescriptor(EImageLayout layout = EImageLayout::E_GENERAL) const noexcept -> SImageDescriptor;

    private:
        VkImage _handle = {};
        VkMemoryRequirements _memoryRequirements = {};
        VkSparseImageMemoryRequirements _sparseMemoryRequirements = {};
        VmaAllocation _allocation = {};
        VmaAllocationInfo _allocationInfo = {};

        CArcPtr<const CImageView> _imageView;

        SImageCreateInfo _createInfo = {};
        CArcPtr<const CDevice> _device;
    };
}
