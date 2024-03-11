#pragma once

#include <Retina/Core/Logger.hpp>
#include <Retina/Core/Macros.hpp>

#define RETINA_WSI_TRACE(...) RETINA_LOGGER_TRACE(::Retina::WSI::GetMainLogger(), __VA_ARGS__)
#define RETINA_WSI_DEBUG(...) RETINA_LOGGER_DEBUG(::Retina::WSI::GetMainLogger(), __VA_ARGS__)
#define RETINA_WSI_INFO(...) RETINA_LOGGER_INFO(::Retina::WSI::GetMainLogger(), __VA_ARGS__)
#define RETINA_WSI_WARN(...) RETINA_LOGGER_WARN(::Retina::WSI::GetMainLogger(), __VA_ARGS__)
#define RETINA_WSI_ERROR(...) RETINA_LOGGER_ERROR(::Retina::WSI::GetMainLogger(), __VA_ARGS__)
#define RETINA_WSI_CRITICAL(...) RETINA_LOGGER_CRITICAL(::Retina::WSI::GetMainLogger(), __VA_ARGS__)
#define RETINA_WSI_PANIC_WITH(message, ...) RETINA_PANIC_WITH(::Retina::WSI::GetMainLogger(), message, __VA_ARGS__)

namespace Retina::WSI {
  RETINA_NODISCARD auto GetMainLogger() noexcept -> Core::CLogger&;
}
