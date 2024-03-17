#include <Retina/Graphics/DescriptorSetInfo.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/ImageView.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>

#include <volk.h>

namespace Retina::Graphics {
  CImageView::CImageView(const CImage& image) noexcept
    : _image(image)
  {
    RETINA_PROFILE_SCOPED();
  }

  CImageView::~CImageView() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      vkDestroyImageView(GetImage().GetDevice().GetHandle(), _handle, nullptr);
      RETINA_GRAPHICS_INFO("Image view ({}) destroyed", GetDebugName());
    }
  }

  auto CImageView::Make(const CImage& image, SImageViewCreateInfo info) noexcept -> Core::CArcPtr<CImageView> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CImageView(image));
    const auto format = info.Format == EResourceFormat::E_UNDEFINED
      ? image.GetFormat()
      : info.Format;
    const auto aspectMask = ImageAspectMaskFrom(format);
    const auto imageViewType = image.GetLayerCount() > 1
      ? VK_IMAGE_VIEW_TYPE_2D_ARRAY
      : VK_IMAGE_VIEW_TYPE_2D;
    auto imageViewCreateInfo = VkImageViewCreateInfo(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
    imageViewCreateInfo.image = image.GetHandle();
    imageViewCreateInfo.viewType = imageViewType;
    imageViewCreateInfo.format = AsEnumCounterpart(format);
    imageViewCreateInfo.components = std::bit_cast<VkComponentMapping>(info.Swizzle);
    imageViewCreateInfo.subresourceRange = MakeNativeImageSubresourceRange(aspectMask, info.SubresourceRange);

    auto imageViewHandle = VkImageView();
    RETINA_GRAPHICS_VULKAN_CHECK(
      vkCreateImageView(
        image.GetDevice().GetHandle(),
        &imageViewCreateInfo,
        nullptr,
        &imageViewHandle
      )
    );
    RETINA_GRAPHICS_INFO("Image view ({}) initialized", info.Name);

    info.Format = format;
    info.SubresourceRange.BaseLevel = imageViewCreateInfo.subresourceRange.baseMipLevel;
    if (info.SubresourceRange.BaseLevel == SUBRESOURCE_LEVEL_IGNORED) {
      info.SubresourceRange.BaseLevel = 0;
    }
    info.SubresourceRange.BaseLayer = imageViewCreateInfo.subresourceRange.baseArrayLayer;
    if (info.SubresourceRange.BaseLevel == SUBRESOURCE_LEVEL_IGNORED) {
      info.SubresourceRange.BaseLevel = 0;
    }
    info.SubresourceRange.LevelCount = imageViewCreateInfo.subresourceRange.levelCount;
    if (info.SubresourceRange.LevelCount == SUBRESOURCE_REMAINING_LEVELS) {
      info.SubresourceRange.LevelCount = image.GetLevelCount() - info.SubresourceRange.BaseLevel;
    }
    info.SubresourceRange.LayerCount = imageViewCreateInfo.subresourceRange.layerCount;
    if (info.SubresourceRange.LayerCount == SUBRESOURCE_REMAINING_LAYERS) {
      info.SubresourceRange.LayerCount = image.GetLayerCount() - info.SubresourceRange.BaseLayer;
    }

    self->_handle = imageViewHandle;
    self->_aspectMask = aspectMask;
    self->SetDebugName(info.Name);
    self->_createInfo = std::move(info);

    return self;
  }

  auto CImageView::GetHandle() const noexcept -> VkImageView {
    RETINA_PROFILE_SCOPED();
    return _handle;
  }

  auto CImageView::GetAspectMask() const noexcept -> EImageAspectFlag {
    RETINA_PROFILE_SCOPED();
    return _aspectMask;
  }

  auto CImageView::GetCreateInfo() const noexcept -> const SImageViewCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CImageView::GetImage() const noexcept -> const CImage& {
    RETINA_PROFILE_SCOPED();
    return _image;
  }

  auto CImageView::GetFormat() const noexcept -> EResourceFormat {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Format;
  }

  auto CImageView::GetSwizzle() const noexcept -> SImageSwizzle {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Swizzle;
  }

  auto CImageView::GetSwizzle(EComponentSwizzle SImageSwizzle::* component) const noexcept -> EComponentSwizzle {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Swizzle.*component;
  }

  auto CImageView::GetSubresourceRange() const noexcept -> const SImageSubresourceRange& {
    RETINA_PROFILE_SCOPED();
    return _createInfo.SubresourceRange;
  }

  auto CImageView::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CImageView::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_SET_DEBUG_NAME(GetImage().GetDevice().GetHandle(), _handle, VK_OBJECT_TYPE_IMAGE_VIEW, name);
    _createInfo.Name = name;
  }

  auto CImageView::GetDescriptor(EImageLayout layout) const noexcept -> SImageDescriptor {
    return {
      .View = _handle,
      .Layout = layout
    };
  }
}