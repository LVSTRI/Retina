#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/ImageInfo.hpp>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Retina {
    class CImageView : public INativeDebugName, public IEnableIntrusiveReferenceCount<CImageView> {
    public:
        using Self = CImageView;

        CImageView(const CImage& image) noexcept;
        ~CImageView() noexcept;

        RETINA_NODISCARD static auto Make(const CImage& image, const SImageViewCreateInfo& createInfo) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkImageView;
        RETINA_NODISCARD auto GetAspectMask() const noexcept -> EImageAspect;
        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SImageViewCreateInfo&;
        RETINA_NODISCARD auto GetImage() const noexcept -> const CImage&;

        RETINA_NODISCARD auto GetFormat() const noexcept -> EResourceFormat;
        RETINA_NODISCARD auto GetSwizzle() const noexcept -> SImageSwizzle;
        RETINA_NODISCARD auto GetSubresourceRange() const noexcept -> const SImageSubresourceRange&;

        auto SetDebugName(std::string_view name) noexcept -> void;

    private:
        VkImageView _handle = {};
        EImageAspect _aspectMask = {};

        SImageViewCreateInfo _createInfo = {};
        std::reference_wrapper<const CImage> _image;
    };
}
