#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Forward.hpp>
#include <Retina/Graphics/Enums.hpp>

#include <compare>
#include <variant>
#include <vector>

namespace Retina::Graphics {
  struct SImageDescriptor {
    // TODO: Samplers
    Core::CReferenceWrapper<const CImageView> View;
    EImageLayout Layout;

    RETINA_NODISCARD RETINA_INLINE constexpr auto operator <=>(const SImageDescriptor&) const noexcept -> std::strong_ordering = default;
  };

  // TODO: Add support for buffer descriptors
  struct SBufferDescriptor {};

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
