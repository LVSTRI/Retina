#include <Retina/Entry/Application.hpp>

#include <filesystem>

namespace Retina {
  extern auto MakeApplication() noexcept -> std::unique_ptr<Entry::IApplication>;
}

namespace Retina::Entry {
  IApplication::~IApplication() noexcept = default;

  auto IApplication::Make() noexcept -> std::unique_ptr<IApplication> {
    RETINA_PROFILE_SCOPED();
    return MakeApplication();
  }
}
