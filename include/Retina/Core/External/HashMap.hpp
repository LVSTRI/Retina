#pragma once

#include <Retina/Core/Core.hpp>

#include <ankerl/unordered_dense.h>

#define RETINA_MAKE_TRANSPARENT_EQUAL_TO_SPECIALIZATION(T)                                              \
    template <>                                                                                         \
    struct ::std::equal_to<T> {                                                                         \
        using is_transparent = void;                                                                    \
        IR_NODISCARD constexpr auto operator ()(const T& lhs, const T& rhs) const noexcept -> bool {    \
            return lhs == rhs;                                                                          \
        }                                                                                               \
    }                                                                                                   \

#define RETINA_MAKE_AVALANCHING_TRANSPARENT_HASH_SPECIALIZATION(T, f)                           \
    template <>                                                                                 \
    struct ::ankerl::unordered_dense::hash<T> {                                                 \
        using is_avalanching = void;                                                            \
        using is_transparent = void;                                                            \
        IR_NODISCARD constexpr auto operator ()(const T& x) const noexcept -> ::std::size_t {   \
            return (f)(x);                                                                      \
        }                                                                                       \
    }

namespace Retina::External {
    using namespace ankerl;
    using namespace ankerl::unordered_dense;

    namespace Wyhash = detail::wyhash;

    template <typename K, typename V>
    using FastHashMap = map<K, V>;
}
