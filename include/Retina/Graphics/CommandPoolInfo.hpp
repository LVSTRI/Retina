#pragma once

#include <Retina/Core/Core.hpp>

#include <string>

namespace Retina {
    struct SCommandPoolCreateInfo {
        std::string Name;
        ECommandPoolCreateFlag Flags = {};

        RETINA_NODISCARD constexpr auto operator <=>(const SCommandPoolCreateInfo& other) const noexcept -> std::strong_ordering = default;
    };

    namespace Constant {
        const inline auto DEFAULT_COMMAND_POOL_INFO = SCommandPoolCreateInfo();
    }
}
