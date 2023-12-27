#pragma once

#include <Retina/Core/Core.hpp>

#include <span>

namespace Retina {
    struct SInstanceFeatureInfo {
        bool Surface = false;
        bool Debug = false;

        RETINA_NODISCARD constexpr auto operator <=>(const SInstanceFeatureInfo&) const noexcept -> std::strong_ordering = default;
    };

    struct SInstanceCreateInfo {
        using PlatformGetSurfaceExtensionsFuncType = auto (*)() noexcept -> std::span<const char*>;

        SInstanceFeatureInfo Features = {};
        PlatformGetSurfaceExtensionsFuncType PlatformGetSurfaceExtensionsFunc = nullptr;

        RETINA_NODISCARD constexpr auto operator <=>(const SInstanceCreateInfo&) const noexcept -> std::strong_ordering = default;
    };
}

