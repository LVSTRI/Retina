#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/ImageInfo.hpp>
#include <Retina/Graphics/ImageView.hpp>

namespace Retina::Graphics {
  auto MakeNativeImageSubresourceRange(
    const CImage& image,
    const SImageSubresourceRange& subresource
  ) noexcept -> VkImageSubresourceRange {
    RETINA_PROFILE_SCOPED();
    auto subresourceRange = VkImageSubresourceRange();
    subresourceRange.aspectMask = AsEnumCounterpart(image.GetView().GetAspectMask());
    subresourceRange.baseMipLevel = subresource.BaseLevel;
    if (subresourceRange.baseMipLevel == SUBRESOURCE_LEVEL_IGNORED) {
      subresourceRange.baseMipLevel = 0;
    }
    subresourceRange.baseArrayLayer = subresource.BaseLayer;
    if (subresourceRange.baseArrayLayer == SUBRESOURCE_LAYER_IGNORED) {
      subresourceRange.baseArrayLayer = 0;
    }
    subresourceRange.levelCount = subresource.LevelCount;
    if (subresourceRange.levelCount == SUBRESOURCE_REMAINING_LEVELS) {
      subresourceRange.levelCount = image.GetLevelCount() - subresourceRange.baseMipLevel;
    }
    subresourceRange.layerCount = subresource.LayerCount;
    if (subresourceRange.layerCount == SUBRESOURCE_REMAINING_LAYERS) {
      subresourceRange.layerCount = image.GetLayerCount() - subresourceRange.baseArrayLayer;
    }
    return subresourceRange;
  }

  auto MakeNativeImageSubresourceLayers(
    const CImage& image,
    const SImageSubresourceRange& subresourceRange
  ) noexcept -> VkImageSubresourceLayers {
    RETINA_PROFILE_SCOPED();
    const auto aspect = image.GetView().GetAspectMask();
    auto subresourceLayers = VkImageSubresourceLayers();
    subresourceLayers.aspectMask = AsEnumCounterpart(aspect);
    subresourceLayers.mipLevel = subresourceRange.BaseLevel;
    if (subresourceLayers.mipLevel == SUBRESOURCE_LEVEL_IGNORED) {
      subresourceLayers.mipLevel = 0;
    }
    subresourceLayers.baseArrayLayer = subresourceRange.BaseLayer;
    if (subresourceLayers.baseArrayLayer == SUBRESOURCE_LAYER_IGNORED) {
      subresourceLayers.baseArrayLayer = 0;
    }
    subresourceLayers.layerCount = subresourceRange.LayerCount;
    if (subresourceLayers.layerCount == SUBRESOURCE_REMAINING_LAYERS) {
      subresourceLayers.layerCount = image.GetLayerCount() - subresourceLayers.baseArrayLayer;
    }
    return subresourceLayers;
  }
}
