#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/WSI/Forward.hpp>
#include <Retina/WSI/Input.hpp>
#include <Retina/WSI/WindowInfo.hpp>

#include <memory>

namespace Retina::WSI {
  class CWindow {
  public:
    using EventDispatcher = Core::CEventDispatcher<
      SWindowFocusEvent,
      SWindowResizeEvent,
      SWindowCloseEvent,
      SWindowKeyboardEvent,
      SWindowMouseButtonEvent,
      SWindowMousePositionEvent,
      SWindowMouseEnterEvent,
      SWindowMouseScrollEvent,
      SWindowInputCharEvent,
      SWindowMonitorEvent
    >;

  public:
    CWindow() noexcept = default;
    ~CWindow() noexcept;
    RETINA_DELETE_COPY(CWindow);
    RETINA_DECLARE_MOVE(CWindow);

    RETINA_NODISCARD static auto Make(const SWindowCreateInfo& createInfo) noexcept -> Core::CUniquePtr<CWindow>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> WindowHandle;
    RETINA_NODISCARD auto GetEventDispatcher() noexcept -> EventDispatcher&;
    RETINA_NODISCARD auto GetInput() noexcept -> CInput&;
    RETINA_NODISCARD auto GetInput() const noexcept -> const CInput&;
    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SWindowCreateInfo&;

    RETINA_NODISCARD auto GetTitle() const noexcept -> std::string_view;
    RETINA_NODISCARD auto GetWidth() const noexcept -> uint32;
    RETINA_NODISCARD auto GetHeight() const noexcept -> uint32;

    RETINA_NODISCARD auto IsFeatureEnabled(bool SWindowFeature::* feature) const noexcept -> bool;

    RETINA_NODISCARD auto IsOpen() const noexcept -> bool;

    RETINA_NODISCARD auto GetPosition() const noexcept -> std::pair<int32, int32>;

  private:
    WindowHandle _handle = nullptr;
    EventDispatcher _dispatcher;
    std::optional<CInput> _input;

    SWindowCreateInfo _createInfo = {};
  };
}
