#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Forward.hpp>
#include <Retina/Graphics/Enum.hpp>

#include <vulkan/vulkan.h>

#include <compare>
#include <variant>
#include <vector>

namespace Retina::Graphics {
  struct SImageDescriptor {
    VkSampler Sampler = {};
    VkImageView View = {};
    EImageLayout Layout = {};

    RETINA_NODISCARD RETINA_INLINE constexpr auto operator <=>(const SImageDescriptor&) const noexcept -> std::strong_ordering = default;
  };

  struct SBufferDescriptor {
    VkBuffer Handle = {};
    VkDeviceMemory Memory = {};
    usize Offset = 0;
    usize Size = 0;
    usize Address = 0;

    RETINA_NODISCARD RETINA_INLINE constexpr auto operator <=>(const SBufferDescriptor&) const noexcept -> std::strong_ordering = default;
  };

  struct SDescriptorWriteInfo {
    uint32 Slot = 0;
    uint32 Binding = -1_u32;
    EDescriptorType Type = {};
    std::variant<
      std::vector<SImageDescriptor>,
      std::vector<SBufferDescriptor>
    > Descriptors;
  };

  struct SDescriptorSetCreateInfo {
    std::string Name;
  };
}
