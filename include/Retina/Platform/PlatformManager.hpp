#pragma once

#include <Retina/Core/Core.hpp>

#include <memory>
#include <span>

namespace Retina::Platform {
    using NativeHandle = void*;
    using SurfaceHandle = void*;

    RETINA_NODISCARD auto MakeWindowHandle(std::string_view title, uint32 width, uint32 height, const SWindowFeatures& features) noexcept -> NativeHandle;
    RETINA_NODISCARD auto IsWindowOpen(NativeHandle handle) noexcept -> bool;
    RETINA_NODISCARD auto GetWindowViewport(NativeHandle handle) noexcept -> std::pair<uint32, uint32>;
    auto DestroyWindowHandle(NativeHandle handle) noexcept -> void;

    RETINA_NODISCARD auto GetSurfaceExtensions() noexcept -> std::span<const char*>;
    RETINA_NODISCARD auto MakeNativeSurface(const CInstance& instance, const CWindow& window) noexcept -> SurfaceHandle;

    auto PollEvents() noexcept -> void;
    auto WaitEvents() noexcept -> void;
}
