#pragma once

#include <Retina/Sandbox/Logger.hpp>

namespace Retina::Sandbox {
  namespace Details {
    static auto MAIN_LOGGER = Core::CLogger::Make("Retina.Sandbox");
  }

  auto GetMainLogger() noexcept -> Core::CLogger& {
    RETINA_PROFILE_SCOPED();
    return Details::MAIN_LOGGER;
  }
}
