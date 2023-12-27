#pragma once

#include <Retina/Core/Core.hpp>

#include <span>

namespace Retina {
    struct SDescriptorLayoutBinding {
        uint32 Count = 0;
        EShaderStage Stage = {};
        EDescriptorType Type = {};
        EDescriptorBindingFlag Flags = {};
    };

    struct SDescriptorLayoutCreateInfo {
        std::string Name;
        EDescriptorLayoutCreateFlag Flags = {};
        std::span<const SDescriptorLayoutBinding> Bindings;
    };
}
