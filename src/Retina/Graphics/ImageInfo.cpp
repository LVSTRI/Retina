#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/ImageInfo.hpp>
#include <Retina/Graphics/ImageView.hpp>

namespace Retina {
    auto MakeNativeImageSubresourceLayers(
        const CImage& image,
        const SImageSubresourceRange& subresource
    ) noexcept -> VkImageSubresourceLayers {
        auto subresourceLayers = VkImageSubresourceLayers();
        subresourceLayers.aspectMask = ToEnumCounterpart(image.GetView().GetAspectMask());
        subresourceLayers.mipLevel = subresource.BaseLevel;
        if (subresourceLayers.mipLevel == Constant::SUBRESOURCE_LEVEL_IGNORED) {
            subresourceLayers.mipLevel = 0;
        }
        subresourceLayers.baseArrayLayer = subresource.BaseLayer;
        if (subresourceLayers.baseArrayLayer == Constant::SUBRESOURCE_LAYER_IGNORED) {
            subresourceLayers.baseArrayLayer = 0;
        }
        subresourceLayers.layerCount = subresource.LayerCount;
        if (subresourceLayers.layerCount == Constant::SUBRESOURCE_REMAINING_LAYERS) {
            subresourceLayers.layerCount = image.GetLayers() - subresourceLayers.baseArrayLayer;
        }
        return subresourceLayers;
    }
}
