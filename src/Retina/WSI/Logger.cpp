#include <Retina/WSI/Logger.hpp>

namespace Retina::WSI {
  namespace Details {
    static auto MAIN_LOGGER = Core::CLogger::Make("Retina.WSI");
  }

  auto GetMainLogger() noexcept -> Core::CLogger& {
    RETINA_PROFILE_SCOPED();
    return Details::MAIN_LOGGER;
  }
}
