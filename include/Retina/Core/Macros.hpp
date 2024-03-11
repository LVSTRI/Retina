#pragma once

#include <cassert>
#include <cstdlib>

#include <debugbreak.h>

#if defined(_WIN32)
  #define RETINA_PLATFORM_WINDOWS
#elif defined(__linux__)
  #define RETINA_PLATFORM_LINUX
#endif

#if !defined(NDEBUG)
  #define RETINA_BUILD_DEBUG
#else
  #define RETINA_BUILD_RELEASE
#endif

#if defined(__clang__)
  #define RETINA_COMPILER_CLANG
#else
  #define RETINA_COMPILER_GCC
#endif

#define RETINA_NODISCARD [[nodiscard]]
#define RETINA_NORETURN [[noreturn]]
#define RETINA_FALLTHROUGH [[fallthrough]]
#define RETINA_MAYBE_UNUSED [[maybe_unused]]
#define RETINA_INLINE __attribute__((always_inline)) inline
#define RETINA_EXTERN_INLINE RETINA_INLINE extern

#define RETINA_CXX_VERSION __cplusplus
#define RETINA_FUNCTION_SIGNATURE() __PRETTY_FUNCTION__

#define RETINA_CONCAT_EXPAND(x, y) x##y
#define RETINA_CONCAT(x, y) RETINA_CONCAT_EXPAND(x, y)

#define RETINA_STRINGIFY_EXPAND(x) #x
#define RETINA_STRINGIFY(x) RETINA_STRINGIFY_EXPAND(x)

#define RETINA_UNUSED(x) (void)(x)

#define RETINA_DECLARE_COPY_CONSTRUCTOR(T, ...) __VA_ARGS__ T(const T&) noexcept
#define RETINA_DECLARE_MOVE_CONSTRUCTOR(T, ...) __VA_ARGS__ T(T&&) noexcept
#define RETINA_DECLARE_COPY_ASSIGNMENT(T, ...) __VA_ARGS__ auto operator =(const T&) noexcept -> T&
#define RETINA_DECLARE_MOVE_ASSIGNMENT(T, ...) __VA_ARGS__ auto operator =(T&&) noexcept -> T&

#define RETINA_DEFAULT_COPY_CONSTRUCTOR(T, ...) __VA_ARGS__ T(const T&) noexcept = default
#define RETINA_DEFAULT_MOVE_CONSTRUCTOR(T, ...) __VA_ARGS__ T(T&&) noexcept = default
#define RETINA_DEFAULT_COPY_ASSIGNMENT(T, ...) __VA_ARGS__ auto operator =(const T&) noexcept -> T& = default
#define RETINA_DEFAULT_MOVE_ASSIGNMENT(T, ...) __VA_ARGS__ auto operator =(T&&) noexcept -> T& = default

#define RETINA_DELETE_COPY_CONSTRUCTOR(T, ...) __VA_ARGS__ T(const T&) noexcept = delete
#define RETINA_DELETE_MOVE_CONSTRUCTOR(T, ...) __VA_ARGS__ T(T&&) noexcept = delete
#define RETINA_DELETE_COPY_ASSIGNMENT(T, ...) __VA_ARGS__ auto operator =(const T&) noexcept -> T& = delete
#define RETINA_DELETE_MOVE_ASSIGNMENT(T, ...) __VA_ARGS__ auto operator =(T&&) noexcept -> T& = delete

#define RETINA_DECLARE_COPY_SWAP(T, ...)          \
  __VA_ARGS__ T(const T&) noexcept;               \
  __VA_ARGS__ T(T&&) noexcept;                    \
  __VA_ARGS__ auto operator =(T) noexcept -> T&

#define RETINA_DECLARE_COPY_SWAP_COPY_ONLY(T, ...)  \
  __VA_ARGS__ T(const T&) noexcept = delete;        \
  __VA_ARGS__ T(T&&) noexcept;                      \
  __VA_ARGS__ auto operator =(T) noexcept -> T&

#define RETINA_DECLARE_COPY_SWAP_MOVE_ONLY(T, ...)  \
  __VA_ARGS__ T(const T&) noexcept = delete;        \
  __VA_ARGS__ T(T&&) noexcept;                      \
  __VA_ARGS__ auto operator =(T) noexcept -> T&

#define RETINA_DECLARE_COPY(T, ...)                \
  RETINA_DECLARE_COPY_CONSTRUCTOR(T, __VA_ARGS__); \
  RETINA_DECLARE_COPY_ASSIGNMENT(T, __VA_ARGS__)

#define RETINA_DECLARE_MOVE(T, ...)                \
  RETINA_DECLARE_MOVE_CONSTRUCTOR(T, __VA_ARGS__); \
  RETINA_DECLARE_MOVE_ASSIGNMENT(T, __VA_ARGS__)

#define RETINA_DECLARE_COPY_MOVE(T, ...)  \
  RETINA_DECLARE_COPY(T, __VA_ARGS__);    \
  RETINA_DECLARE_MOVE(T, __VA_ARGS__)

#define RETINA_DEFAULT_COPY(T, ...)                \
  RETINA_DEFAULT_COPY_CONSTRUCTOR(T, __VA_ARGS__); \
  RETINA_DEFAULT_COPY_ASSIGNMENT(T, __VA_ARGS__)

#define RETINA_DEFAULT_MOVE(T, ...)                \
  RETINA_DEFAULT_MOVE_CONSTRUCTOR(T, __VA_ARGS__); \
  RETINA_DEFAULT_MOVE_ASSIGNMENT(T, __VA_ARGS__)

#define RETINA_DEFAULT_COPY_MOVE(T, ...)  \
  RETINA_DEFAULT_COPY(T, __VA_ARGS__);    \
  RETINA_DEFAULT_MOVE(T, __VA_ARGS__)

#define RETINA_DELETE_COPY(T, ...)                \
  RETINA_DELETE_COPY_CONSTRUCTOR(T, __VA_ARGS__); \
  RETINA_DELETE_COPY_ASSIGNMENT(T, __VA_ARGS__)

#define RETINA_DELETE_MOVE(T, ...)                \
  RETINA_DELETE_MOVE_CONSTRUCTOR(T, __VA_ARGS__); \
  RETINA_DELETE_MOVE_ASSIGNMENT(T, __VA_ARGS__)

#define RETINA_DELETE_COPY_MOVE(T, ...) \
  RETINA_DELETE_COPY(T, __VA_ARGS__);   \
  RETINA_DELETE_MOVE(T, __VA_ARGS__)

#if defined(RETINA_BUILD_DEBUG)
  #define RETINA_ASSERT(expression) assert(expression)
  #define RETINA_ASSERT_WITH(expression, message) assert((expression) && (message))
  #define RETINA_DEBUG_BREAK() debug_break()
#else
  #define RETINA_ASSERT(expression) (void)(expression)
  #define RETINA_ASSERT_WITH(expression, message) ((void)(expression), (void)(message))
  #define RETINA_DEBUG_BREAK()
#endif

#define RETINA_PANIC() std::terminate()
#define RETINA_PANIC_WITH(logger, message, ...)           \
  do {                                                    \
    (logger).Critical(message __VA_OPT__(,) __VA_ARGS__); \
    RETINA_PANIC();                                       \
  } while (false)

#if defined(RETINA_ENABLE_PROFILER)
  #ifndef TRACY_ENABLE
    #define TRACY_ENABLE
  #endif
  #include <tracy/Tracy.hpp>

  #define RETINA_PROFILE_NAMED_SCOPE(name) ZoneScopedN(name)
  #define RETINA_PROFILE_SCOPED() ZoneScoped
  #define RETINA_MARK_FRAME() FrameMark
#else
  #define RETINA_PROFILE_NAMED_SCOPE(name) (void)(name)
  #define RETINA_PROFILE_SCOPED() RETINA_UNUSED(0)
  #define RETINA_MARK_FRAME() RETINA_UNUSED(0)
#endif
