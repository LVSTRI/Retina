#pragma once

#include <Retina/Core/Core.hpp>

namespace Retina::Entry {
  struct IApplication {
    virtual ~IApplication() noexcept = 0;

    RETINA_NODISCARD static auto Make() noexcept -> std::unique_ptr<IApplication>;

    virtual auto Run() noexcept -> void = 0;
  };
}
