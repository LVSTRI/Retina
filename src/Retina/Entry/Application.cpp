#include <Retina/Entry/Application.hpp>

#include <filesystem>

namespace Retina {
  extern auto MakeApplication() noexcept -> Core::CUniquePtr<Entry::IApplication>;
}

namespace Retina::Entry {
  IApplication::~IApplication() noexcept = default;

  auto IApplication::Make() noexcept -> Core::CUniquePtr<IApplication> {
    RETINA_PROFILE_SCOPED();
    return MakeApplication();
  }
}
