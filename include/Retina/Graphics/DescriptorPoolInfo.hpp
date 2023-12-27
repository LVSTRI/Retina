#pragma once

#include <Retina/Core/Core.hpp>

namespace Retina {
    struct SDescriptorPoolSize {
        EDescriptorType Type = {};
        uint32 Count = 0;
    };

    struct SDescriptorPoolCreateInfo {
        std::string Name;
        EDescriptorPoolCreateFlag Flags = {};
        std::vector<SDescriptorPoolSize> Sizes;
    };
}
