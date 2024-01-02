#include <Retina/Platform/Window.hpp>

namespace Retina {
    CWindow::CWindow() noexcept : _input(*this) {
        RETINA_PROFILE_SCOPED();
    }

    CWindow::~CWindow() noexcept {
        RETINA_PROFILE_SCOPED();
        Platform::DestroyWindowHandle(_handle);
    }

    auto CWindow::Make(const SWindowCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto window = CArcPtr(new Self());
        const auto windowHandle = Platform::MakeWindowHandle(createInfo.Title, createInfo.Width, createInfo.Height, createInfo.Features);
        window->_width = createInfo.Width;
        window->_height = createInfo.Height;
        window->_createInfo = createInfo;
        window->_handle = windowHandle;
        return window;
    }

    auto CWindow::GetHandle() const noexcept -> Platform::NativeHandle {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto CWindow::GetTitle() const noexcept -> std::string_view {
        RETINA_PROFILE_SCOPED();
        return _createInfo.Title;
    }

    auto CWindow::GetWidth() const noexcept -> uint32 {
        RETINA_PROFILE_SCOPED();
        return _width;
    }

    auto CWindow::GetHeight() const noexcept -> uint32 {
        RETINA_PROFILE_SCOPED();
        return _height;
    }

    auto CWindow::GetInput() noexcept -> CInput& {
        RETINA_PROFILE_SCOPED();
        return _input;
    }

    auto CWindow::GetInput() const noexcept -> const CInput& {
        RETINA_PROFILE_SCOPED();
        return _input;
    }

    auto CWindow::GetCreateInfo() const noexcept -> const SWindowCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CWindow::GetFeatures() const noexcept -> const SWindowFeatures& {
        RETINA_PROFILE_SCOPED();
        return _createInfo.Features;
    }

    auto CWindow::IsOpen() const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        return Platform::IsWindowOpen(_handle);
    }

    auto CWindow::UpdateViewportExtent() noexcept -> void {
        RETINA_PROFILE_SCOPED();
        const auto [width, height] = Platform::GetWindowViewport(_handle);
        _width = width;
        _height = height;
    }
}
