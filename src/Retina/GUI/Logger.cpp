#include <Retina/GUI/Logger.hpp>

namespace Retina::GUI {
  namespace Details {
    static auto MAIN_LOGGER = Core::CLogger::Make("Retina.GUI");
  }

  auto GetMainLogger() noexcept -> Core::CLogger& {
    RETINA_PROFILE_SCOPED();
    return Details::MAIN_LOGGER;
  }
}
