#pragma once

#include <Retina/Core/Core.hpp>

#include <span>

namespace Retina {
    template <typename T>
    struct SBufferUploadInfo {
        std::string Name;
        std::span<const T> Data;
    };

    struct SBufferCreateInfo {
        std::string Name;
        EBufferCreateFlag Flags = {};
        EMemoryProperty Heap = EMemoryProperty::E_DEVICE_LOCAL;
        uint64 Capacity = 0;
    };
}