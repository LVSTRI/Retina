#pragma once

#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Types.hpp>

#include <spdlog/spdlog.h>

#include <memory>
#include <format>

#if defined(RETINA_ENABLE_LOGGER)
  #define RETINA_LOGGER_TRACE(f, ...) f.Trace(__VA_ARGS__)
  #define RETINA_LOGGER_DEBUG(f, ...) f.Debug(__VA_ARGS__)
  #define RETINA_LOGGER_INFO(f, ...) f.Info(__VA_ARGS__)
  #define RETINA_LOGGER_WARN(f, ...) f.Warn(__VA_ARGS__)
  #define RETINA_LOGGER_ERROR(f, ...) f.Error(__VA_ARGS__)
  #define RETINA_LOGGER_CRITICAL(f, ...) f.Critical(__VA_ARGS__)
#else
  #define RETINA_LOGGER_TRACE(f, ...) RETINA_UNUSED(f, __VA_ARGS__)
  #define RETINA_LOGGER_DEBUG(f, ...) RETINA_UNUSED(f, __VA_ARGS__)
  #define RETINA_LOGGER_INFO(f, ...) RETINA_UNUSED(f, __VA_ARGS__)
  #define RETINA_LOGGER_WARN(f, ...) RETINA_UNUSED(f, __VA_ARGS__)
  #define RETINA_LOGGER_ERROR(f, ...) RETINA_UNUSED(f, __VA_ARGS__)
  #define RETINA_LOGGER_CRITICAL(f, ...) RETINA_UNUSED(f, __VA_ARGS__)
#endif

#define RETINA_CORE_TRACE(...) RETINA_LOGGER_TRACE(::Retina::Core::GetMainLogger(), __VA_ARGS__)
#define RETINA_CORE_DEBUG(...) RETINA_LOGGER_DEBUG(::Retina::Core::GetMainLogger(), __VA_ARGS__)
#define RETINA_CORE_INFO(...) RETINA_LOGGER_INFO(::Retina::Core::GetMainLogger(), __VA_ARGS__)
#define RETINA_CORE_WARN(...) RETINA_LOGGER_WARN(::Retina::Core::GetMainLogger(), __VA_ARGS__)
#define RETINA_CORE_ERROR(...) RETINA_LOGGER_ERROR(::Retina::Core::GetMainLogger(), __VA_ARGS__)
#define RETINA_CORE_CRITICAL(...) RETINA_LOGGER_CRITICAL(::Retina::Core::GetMainLogger(), __VA_ARGS__)
#define RETINA_CORE_PANIC_WITH(message, ...) RETINA_PANIC_WITH(::Retina::Core::GetMainLogger(), message, __VA_ARGS__)

namespace Retina::Core {
  class CLogger {
  public:
    CLogger() noexcept = default;
    ~CLogger() noexcept = default;
    RETINA_DEFAULT_COPY_MOVE(CLogger);

    RETINA_NODISCARD static auto Make(std::string_view name) noexcept -> CLogger;

    RETINA_NODISCARD auto GetHandle() const noexcept -> const spdlog::logger&;

    template <typename... Args>
    auto Trace(fmt::format_string<Args...> format, Args&&... args) noexcept -> void;

    template <typename... Args>
    auto Debug(fmt::format_string<Args...> format, Args&&... args) noexcept -> void;

    template <typename... Args>
    auto Info(fmt::format_string<Args...> format, Args&&... args) noexcept -> void;

    template <typename... Args>
    auto Warn(fmt::format_string<Args...> format, Args&&... args) noexcept -> void;

    template <typename... Args>
    auto Error(fmt::format_string<Args...> format, Args&&... args) noexcept -> void;

    template <typename... Args>
    auto Critical(fmt::format_string<Args...> format, Args&&... args) noexcept -> void;

  private:
    std::shared_ptr<spdlog::logger> _handle;
  };

  auto GetMainLogger() noexcept -> CLogger&;

  template <typename... Args>
  auto CLogger::Trace(fmt::format_string<Args...> format, Args&& ... args) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _handle->trace(format, std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto CLogger::Debug(fmt::format_string<Args...> format, Args&& ... args) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _handle->debug(format, std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto CLogger::Info(fmt::format_string<Args...> format, Args&& ... args) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _handle->info(format, std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto CLogger::Warn(fmt::format_string<Args...> format, Args&& ... args) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _handle->warn(format, std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto CLogger::Error(fmt::format_string<Args...> format, Args&& ... args) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _handle->error(format, std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto CLogger::Critical(fmt::format_string<Args...> format, Args&& ... args) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _handle->critical(format, std::forward<Args>(args)...);
  }
}
