#pragma once

#include <Retina/Core/Core.hpp>

#include <vulkan/vulkan.h>

namespace Retina {
    enum class ESemaphoreKind {
        E_BINARY,
        E_TIMELINE
    };

    struct SBinarySemaphoreCreateInfo {
        std::string Name;

        RETINA_NODISCARD constexpr auto operator <=>(const SBinarySemaphoreCreateInfo& other) const noexcept -> std::strong_ordering = default;
    };

    struct STimelineSemaphoreCreateInfo {
        std::string Name;
        uint64 Counter = 0;

        RETINA_NODISCARD constexpr auto operator <=>(const STimelineSemaphoreCreateInfo& other) const noexcept -> std::strong_ordering = default;
    };
}
