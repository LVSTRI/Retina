#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Platform/PlatformManager.hpp>
#include <Retina/Platform/WindowInfo.hpp>

#include <spdlog/spdlog.h>

namespace Retina {
    class CWindow : public IEnableIntrusiveReferenceCount<CWindow> {
    public:
        using Self = CWindow;

        CWindow() noexcept = default;
        ~CWindow() noexcept;

        RETINA_NODISCARD static auto Make(const SWindowCreateInfo& createInfo) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> Platform::NativeHandle;
        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SWindowCreateInfo&;
        RETINA_NODISCARD auto GetTitle() const noexcept -> std::string_view;
        RETINA_NODISCARD auto GetWidth() const noexcept -> uint32;
        RETINA_NODISCARD auto GetHeight() const noexcept -> uint32;
        RETINA_NODISCARD auto GetFeatures() const noexcept -> const SWindowFeatures&;

        RETINA_NODISCARD auto IsOpen() const noexcept -> bool;
        auto UpdateViewportExtent() noexcept -> void;

    private:
        Platform::NativeHandle _handle = {};
        uint32 _width = 0;
        uint32 _height = 0;

        SWindowCreateInfo _createInfo = {};
        std::shared_ptr<spdlog::logger> _logger;
    };
}
