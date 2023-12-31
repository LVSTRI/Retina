#pragma once

#include <Retina/Core/Core.hpp>

#include <variant>

namespace Retina {
    struct SImageDescriptor {
        VkSampler Sampler = {};
        VkImageView View = {};
        EImageLayout Layout = {};

        RETINA_NODISCARD constexpr auto operator <=>(const SImageDescriptor&) const noexcept -> std::strong_ordering = default;
    };

    struct SBufferDescriptor {
        VkBuffer Handle = {};
        VkDeviceMemory Memory = {};
        uint64 Offset = 0;
        uint64 Size = 0;
        uint64 Address = 0;

        RETINA_NODISCARD constexpr auto operator <=>(const SBufferDescriptor&) const noexcept -> std::strong_ordering = default;
    };

    struct SAccelerationStructureDescriptor {
        VkAccelerationStructureKHR Handle = {};

        RETINA_NODISCARD constexpr auto operator <=>(const SAccelerationStructureDescriptor&) const noexcept -> std::strong_ordering = default;
    };

    struct SDescriptorWriteInfo {
        uint32 Slot = 0;
        uint32 Binding = -1_u32;
        EDescriptorType Type = {};
        std::variant<
            std::vector<SImageDescriptor>,
            std::vector<SBufferDescriptor>,
            std::vector<SAccelerationStructureDescriptor>
        > Descriptors;

        RETINA_NODISCARD constexpr auto operator <=>(const SDescriptorWriteInfo&) const noexcept -> std::strong_ordering = default;
    };

    struct SDescriptorSetCreateInfo {
        std::string Name;
        CArcPtr<const CDescriptorLayout> Layout;
    };
}
