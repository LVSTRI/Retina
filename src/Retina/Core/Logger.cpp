#include <Retina/Core/Logger.hpp>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/pattern_formatter.h>

namespace Retina::Core {
  namespace Details {
    static auto MAIN_LOGGER = CLogger::Make("Retina.Core");

    class CLogLevelFormatter : public spdlog::custom_flag_formatter {
    public:
      CLogLevelFormatter() noexcept
        : spdlog::custom_flag_formatter()
      {
        RETINA_PROFILE_SCOPED();
      }

      ~CLogLevelFormatter() noexcept override = default;

      auto format(
        const spdlog::details::log_msg& message,
        const std::tm&,
        spdlog::memory_buf_t& dest
      ) noexcept -> void override {
        RETINA_PROFILE_SCOPED();
        switch (message.level) {
          case spdlog::level::trace:
            dest.append(std::string_view("Trace"));
            break;
          case spdlog::level::debug:
            dest.append(std::string_view("Debug"));
            break;
          case spdlog::level::info:
            dest.append(std::string_view("Info"));
            break;
          case spdlog::level::warn:
            dest.append(std::string_view("Warning"));
            break;
          case spdlog::level::err:
            dest.append(std::string_view("Error"));
            break;
          case spdlog::level::critical:
            dest.append(std::string_view("Critical"));
            break;
          case spdlog::level::off:
            dest.append(std::string_view("Off"));
            break;
          default:
            dest.append(std::string_view("Unknown"));
            break;
        }
      }

      auto clone() const noexcept -> std::unique_ptr<spdlog::custom_flag_formatter> override {
        RETINA_PROFILE_SCOPED();
        return spdlog::details::make_unique<CLogLevelFormatter>();
      }
    };
  }

  auto CLogger::Make(std::string_view name) noexcept -> CLogger {
    RETINA_PROFILE_SCOPED();
    auto self = CLogger();
    auto sink = spdlog::stdout_color_mt(name.data());
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<Details::CLogLevelFormatter>('L');
    formatter->set_pattern("%^[%Y-%m-%d %T.%e] [%n] [%L]:%$ %v");
    sink->set_level(spdlog::level::trace);
    sink->set_formatter(std::move(formatter));

    self._handle = std::move(sink);
    return self;
  }

  auto CLogger::GetHandle() const noexcept -> const spdlog::logger& {
    RETINA_PROFILE_SCOPED();
    return *_handle;
  }

  auto GetMainLogger() noexcept -> CLogger& {
    RETINA_PROFILE_SCOPED();
    return Details::MAIN_LOGGER;
  }
}
