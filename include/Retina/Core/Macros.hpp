#pragma once

#include <cassert>
#include <cstdlib>

#include <vulkan/vk_enum_string_helper.h>

#define RETINA_NODISCARD [[nodiscard]]
#define RETINA_NORETURN [[noreturn]]
#define RETINA_FALLTHROUGH [[fallthrough]]
#define RETINA_MAYBE_UNUSED [[maybe_unused]]
#define RETINA_INLINE inline
#define RETINA_CONSTEXPR constexpr

#if defined(__clang__) || defined(__GNUC__)
    #define RETINA_CPP_VERSION __cplusplus
    #define RETINA_SIGNATURE() __PRETTY_FUNCTION__
#else
    #error "unsupported"
#endif

#define RETINA_CONCAT_EXPAND(x, y) x##y
#define RETINA_CONCAT(x, y) RETINA_CONCAT_EXPAND(x, y)

#if defined(RETINA_DEBUG_LOGGER)
    #define RETINA_LOG_DEBUG(logger, ...) (logger).debug(__VA_ARGS__)
    #define RETINA_LOG_INFO(logger, ...) (logger).info(__VA_ARGS__)
    #define RETINA_LOG_WARN(logger, ...) (logger).warn(__VA_ARGS__)
    #define RETINA_LOG_ERROR(logger, ...) (logger).error(__VA_ARGS__)
    #define RETINA_LOG_CRITICAL(logger, ...) (logger).critical(__VA_ARGS__)
#else
    #define RETINA_LOG_DEBUG(logger, ...) (void)(logger)
    #define RETINA_LOG_INFO(logger, ...) (void)(logger)
    #define RETINA_LOG_WARN(logger, ...) (void)(logger)
    #define RETINA_LOG_ERROR(logger, ...) (void)(logger)
    #define RETINA_LOG_CRITICAL(logger, ...) (void)(logger)
#endif

#if defined(RETINA_DEBUG)
    #define RETINA_ASSERT(expr) assert(expr)
    #define RETINA_ASSERT_WITH(expr, message) RETINA_ASSERT((expr) && (message))
    #define RETINA_VULKAN_CHECK(logger, expr)                                               \
        do {                                                                                \
            const auto __result = (expr);                                                   \
            if (__result != VK_SUCCESS) {                                                   \
                RETINA_LOG_CRITICAL(logger, "Vulkan Error: {}", string_VkResult(__result)); \
                RETINA_ASSERT(false);                                                       \
            }                                                                               \
        } while (false)
#else
    #define RETINA_ASSERT(expr) (void)(expr)
    #define RETINA_ASSERT_WITH(expr, message) (void)(expr)
    #define RETINA_VULKAN_CHECK(logger, result) (void)(result)
#endif

#define RETINA_PANIC() std::abort()
#define RETINA_PANIC_WITH(logger, ...)  \
    do {                                \
        (logger).critical(__VA_ARGS__); \
        RETINA_PANIC();                 \
    } while (false)

#if defined(RETINA_DEBUG_PROFILER)
    #include <tracy/Tracy.hpp>

    #define RETINA_PROFILE_NAMED_SCOPE(name) ZoneScopedN(name)
    #define RETINA_PROFILE_SCOPED() ZoneScoped
    #define RETINA_MARK_FRAME() FrameMark
#else
    #define RETINA_PROFILE_NAMED_SCOPE(name) (void)(name)
    #define RETINA_PROFILE_SCOPED()
    #define RETINA_MARK_FRAME()
#endif

#define RETINA_MEMBER_INDEX(self, member) offsetof(self, member) / sizeof(self::member)

#define RETINA_STATIC_DISPATCH_CASE(x, y)   \
    case (x): {                             \
        (y);                                \
    } break;

#define RETINA_STATIC_DISPATCH(x, ...)  \
    switch (x) {                        \
        __VA_ARGS__                     \
    }
