#pragma once

#include <Retina/Core/Core.hpp>

#include <string>

namespace Retina {
    struct SSwapchainCreateInfo {
        using MakeSurfaceFuncType = auto (*)(const CInstance&, const CWindow&) noexcept -> void*;

        std::string Name;
        bool VSync = false;
        MakeSurfaceFuncType MakeSurfaceFunc = nullptr;

        RETINA_NODISCARD constexpr auto operator <=>(const SSwapchainCreateInfo&) const noexcept -> std::strong_ordering = default;
    };
}
