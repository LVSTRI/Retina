#pragma once

#include <Retina/Core/Core.hpp>

namespace Retina {
    struct SWindowFeatures {
        bool Resizable = false;
        bool Decorated = true;
        bool Focused = true;

        RETINA_NODISCARD constexpr auto operator <=>(const SWindowFeatures&) const noexcept -> std::strong_ordering = default;
    };

    struct SWindowCreateInfo {
        std::string_view Title;
        uint32 Width = 0;
        uint32 Height = 0;
        SWindowFeatures Features = {};

        RETINA_NODISCARD constexpr auto operator <=>(const SWindowCreateInfo&) const noexcept -> std::strong_ordering = default;
    };
}
