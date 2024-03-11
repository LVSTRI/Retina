#include <Retina/Graphics/Logger.hpp>

namespace Retina::Graphics {
  namespace Details {
    static auto MAIN_LOGGER = Core::CLogger::Make("Retina.Graphics");
  }

  auto GetMainLogger() noexcept -> Core::CLogger& {
    RETINA_PROFILE_SCOPED();
    return Details::MAIN_LOGGER;
  }
}
