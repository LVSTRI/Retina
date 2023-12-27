#pragma once

#include <Retina/Core/Core.hpp>

namespace Retina {
    struct SFenceCreateInfo {
        std::string Name;
        bool IsSignaled = false;

        RETINA_NODISCARD constexpr auto operator <=>(const SFenceCreateInfo& other) const noexcept -> std::strong_ordering = default;
    };
}
