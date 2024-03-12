#pragma once

#include <Retina/Graphics/Forward.hpp>
#include <Retina/Graphics/ImageInfo.hpp>

#include <vulkan/vulkan.h>

namespace Retina::Graphics {
  class CImageView : public Core::IEnableIntrusiveReferenceCount<CImageView> {
  public:
    CImageView(const CImage& image) noexcept;
    ~CImageView() noexcept;

    RETINA_NODISCARD static auto Make(const CImage& image, SImageViewCreateInfo info) noexcept -> Core::CArcPtr<CImageView>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkImageView;
    RETINA_NODISCARD auto GetAspectMask() const noexcept -> EImageAspectFlag;
    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SImageViewCreateInfo&;
    RETINA_NODISCARD auto GetImage() const noexcept -> const CImage&;

    RETINA_NODISCARD auto GetFormat() const noexcept -> EResourceFormat;
    RETINA_NODISCARD auto GetSwizzle() const noexcept -> SImageSwizzle;
    RETINA_NODISCARD auto GetSwizzle(EComponentSwizzle SImageSwizzle::* component) const noexcept -> EComponentSwizzle;
    RETINA_NODISCARD auto GetSubresourceRange() const noexcept -> const SImageSubresourceRange&;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

  private:
    VkImageView _handle = {};
    EImageAspectFlag _aspectMask = {};

    SImageViewCreateInfo _createInfo = {};
    Core::CReferenceWrapper<const CImage> _image;
  };
}
