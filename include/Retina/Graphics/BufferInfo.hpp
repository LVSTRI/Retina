#pragma once

#include <Retina/Core/Core.hpp>

namespace Retina {
    namespace Constant {
        constexpr auto HEAP_TYPE_DEVICE_ONLY = EMemoryProperty::E_DEVICE_LOCAL;

        constexpr auto HEAP_TYPE_DEVICE_MAPPABLE = EMemoryProperty::E_DEVICE_LOCAL |
                                                   EMemoryProperty::E_HOST_VISIBLE |
                                                   EMemoryProperty::E_HOST_COHERENT;

        constexpr auto HEAP_TYPE_HOST_ONLY_COHERENT = EMemoryProperty::E_HOST_VISIBLE |
                                                      EMemoryProperty::E_HOST_COHERENT;

        constexpr auto HEAP_TYPE_HOST_ONLY_CACHED = EMemoryProperty::E_HOST_VISIBLE |
                                                    EMemoryProperty::E_HOST_CACHED;
    }

    struct SBufferCreateInfo {
        std::string Name;
        EBufferCreateFlag Flags = {};
        EMemoryProperty Heap = Constant::HEAP_TYPE_DEVICE_ONLY;
        uint64 Capacity = 0;
    };
}