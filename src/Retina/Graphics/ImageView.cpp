#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/ImageView.hpp>

#include <volk.h>

namespace Retina {
    CImageView::CImageView(const CImage& image) noexcept : _image(image) {
        RETINA_PROFILE_SCOPED();
    }

    CImageView::~CImageView() noexcept {
        RETINA_PROFILE_SCOPED();
        const auto& device = _image.get().GetDevice();
        RETINA_LOG_INFO(device.GetLogger(), "Destroying ImageView \"{}\"", GetDebugName());
        vkDestroyImageView(device.GetHandle(), _handle, nullptr);
    }

    auto CImageView::Make(const CImage& image, const SImageViewCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto imageView = CArcPtr(new Self(image));
        RETINA_LOG_INFO(image.GetDevice().GetLogger(), "Creating ImageView \"{}\" for \"{}\"", createInfo.Name, image.GetDebugName());
        const auto aspectMask = ImageAspectMaskFromFormat(createInfo.Format);
        const auto imageViewType = image.GetLayers() > 1
            ? VK_IMAGE_VIEW_TYPE_2D_ARRAY
            : VK_IMAGE_VIEW_TYPE_2D;
        auto imageViewCreateInfo = VkImageViewCreateInfo(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
        imageViewCreateInfo.image = image.GetHandle();
        imageViewCreateInfo.viewType = imageViewType;
        imageViewCreateInfo.format = createInfo.Format == EResourceFormat::E_UNDEFINED
            ? ToEnumCounterpart(image.GetFormat())
            : ToEnumCounterpart(createInfo.Format);
        imageViewCreateInfo.components = VkComponentMapping(
            ToEnumCounterpart(createInfo.Swizzle.R),
            ToEnumCounterpart(createInfo.Swizzle.G),
            ToEnumCounterpart(createInfo.Swizzle.B),
            ToEnumCounterpart(createInfo.Swizzle.A)
        );
        imageViewCreateInfo.subresourceRange = MakeNativeImageSubresourceRange(aspectMask, createInfo.Subresource);
        auto imageViewHandle = VkImageView();
        const auto& device = image.GetDevice();
        RETINA_VULKAN_CHECK(device.GetLogger(), vkCreateImageView(device.GetHandle(), &imageViewCreateInfo, nullptr, &imageViewHandle));

        imageView->_handle = imageViewHandle;
        imageView->_aspectMask = aspectMask;
        imageView->_createInfo = createInfo;
        imageView->SetDebugName(createInfo.Name);
        return imageView;
    }

    auto CImageView::GetHandle() const noexcept -> VkImageView {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto CImageView::GetAspectMask() const noexcept -> EImageAspect {
        RETINA_PROFILE_SCOPED();
        return _aspectMask;
    }

    auto CImageView::GetCreateInfo() const noexcept -> const SImageViewCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CImageView::GetImage() const noexcept -> const CImage& {
        RETINA_PROFILE_SCOPED();
        return _image.get();
    }

    auto CImageView::GetFormat() const noexcept -> EResourceFormat {
        RETINA_PROFILE_SCOPED();
        return _createInfo.Format;
    }

    auto CImageView::GetSwizzle() const noexcept -> SImageSwizzle {
        RETINA_PROFILE_SCOPED();
        return _createInfo.Swizzle;
    }

    auto CImageView::GetSubresourceRange() const noexcept -> const SImageSubresourceRange& {
        RETINA_PROFILE_SCOPED();
        return _createInfo.Subresource;
    }

    auto CImageView::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        const auto& device = _image.get().GetDevice();
        RETINA_VULKAN_CHECK(device.GetLogger(), vkSetDebugUtilsObjectNameEXT(device.GetHandle(), &info));
    }
}
