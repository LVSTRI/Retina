#pragma once

#include <Retina/Core/Logger.hpp>
#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Types.hpp>

#define RETINA_GUI_TRACE(...) RETINA_LOGGER_TRACE(::Retina::GUI::GetMainLogger(), __VA_ARGS__)
#define RETINA_GUI_DEBUG(...) RETINA_LOGGER_DEBUG(::Retina::GUI::GetMainLogger(), __VA_ARGS__)
#define RETINA_GUI_INFO(...) RETINA_LOGGER_INFO(::Retina::GUI::GetMainLogger(), __VA_ARGS__)
#define RETINA_GUI_WARN(...) RETINA_LOGGER_WARN(::Retina::GUI::GetMainLogger(), __VA_ARGS__)
#define RETINA_GUI_ERROR(...) RETINA_LOGGER_ERROR(::Retina::GUI::GetMainLogger(), __VA_ARGS__)
#define RETINA_GUI_CRITICAL(...) RETINA_LOGGER_CRITICAL(::Retina::GUI::GetMainLogger(), __VA_ARGS__)
#define RETINA_GUI_PANIC_WITH(message, ...) RETINA_PANIC_WITH(::Retina::GUI::GetMainLogger(), message, __VA_ARGS__)

namespace Retina::GUI {
  auto GetMainLogger() noexcept -> Core::CLogger&;
}
