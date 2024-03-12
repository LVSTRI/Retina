#pragma once

#include <Retina/Core/Logger.hpp>
#include <Retina/Core/Macros.hpp>

#define RETINA_GRAPHICS_TRACE(...) RETINA_LOGGER_TRACE(::Retina::Graphics::GetMainLogger(), __VA_ARGS__)
#define RETINA_GRAPHICS_DEBUG(...) RETINA_LOGGER_DEBUG(::Retina::Graphics::GetMainLogger(), __VA_ARGS__)
#define RETINA_GRAPHICS_INFO(...) RETINA_LOGGER_INFO(::Retina::Graphics::GetMainLogger(), __VA_ARGS__)
#define RETINA_GRAPHICS_WARN(...) RETINA_LOGGER_WARN(::Retina::Graphics::GetMainLogger(), __VA_ARGS__)
#define RETINA_GRAPHICS_ERROR(...) RETINA_LOGGER_ERROR(::Retina::Graphics::GetMainLogger(), __VA_ARGS__)
#define RETINA_GRAPHICS_CRITICAL(...) RETINA_LOGGER_CRITICAL(::Retina::Graphics::GetMainLogger(), __VA_ARGS__)
#define RETINA_GRAPHICS_PANIC_WITH(message, ...) RETINA_PANIC_WITH(::Retina::Graphics::GetMainLogger(), message, __VA_ARGS__)

namespace Retina::Graphics {
  auto GetMainLogger() noexcept -> Core::CLogger&;
}
