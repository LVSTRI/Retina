#pragma once

#include <Retina/Core/Logger.hpp>
#include <Retina/Core/Macros.hpp>

#define RETINA_SANDBOX_TRACE(...) RETINA_LOGGER_TRACE(::Retina::Sandbox::GetMainLogger(), __VA_ARGS__)
#define RETINA_SANDBOX_DEBUG(...) RETINA_LOGGER_DEBUG(::Retina::Sandbox::GetMainLogger(), __VA_ARGS__)
#define RETINA_SANDBOX_INFO(...) RETINA_LOGGER_INFO(::Retina::Sandbox::GetMainLogger(), __VA_ARGS__)
#define RETINA_SANDBOX_WARN(...) RETINA_LOGGER_WARN(::Retina::Sandbox::GetMainLogger(), __VA_ARGS__)
#define RETINA_SANDBOX_ERROR(...) RETINA_LOGGER_ERROR(::Retina::Sandbox::GetMainLogger(), __VA_ARGS__)
#define RETINA_SANDBOX_CRITICAL(...) RETINA_LOGGER_CRITICAL(::Retina::Sandbox::GetMainLogger(), __VA_ARGS__)
#define RETINA_SANDBOX_PANIC_WITH(message, ...) RETINA_PANIC_WITH(::Retina::Sandbox::GetMainLogger(), message, __VA_ARGS__)

namespace Retina::Sandbox {
  auto GetMainLogger() noexcept -> Core::CLogger&;
}
