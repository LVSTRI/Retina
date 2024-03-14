#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/WSI/Forward.hpp>
#include <Retina/WSI/WindowInfo.hpp>

namespace Retina::WSI {
  class CInput {
  public:
    using EventDispatcher = Core::CEventDispatcher<SInputCursorModeEvent>;

  public:
    CInput(CWindow& window) noexcept;
    ~CInput() noexcept = default;
    RETINA_DELETE_COPY(CInput);
    RETINA_DECLARE_MOVE(CInput);

    RETINA_NODISCARD static auto Make(CWindow& window) noexcept -> CInput;

    RETINA_NODISCARD auto GetEventDispatcher() noexcept -> EventDispatcher&;

    RETINA_NODISCARD auto IsKeyPressed(EInputKeyboard key) const noexcept -> bool;
    RETINA_NODISCARD auto IsKeyReleased(EInputKeyboard key) const noexcept -> bool;
    RETINA_NODISCARD auto IsKeyRepeated(EInputKeyboard key) const noexcept -> bool;

    RETINA_NODISCARD auto IsMouseButtonPressed(EInputMouse button) const noexcept -> bool;
    RETINA_NODISCARD auto IsMouseButtonReleased(EInputMouse button) const noexcept -> bool;
    RETINA_NODISCARD auto IsMouseButtonRepeated(EInputMouse button) const noexcept -> bool;

    RETINA_NODISCARD auto GetCursorPosition() const noexcept -> SInputCursorPosition;

    auto SetCursorMode(EInputCursorMode mode) noexcept -> void;

  private:
    EventDispatcher _dispatcher;

    Core::CReferenceWrapper<CWindow> _window;
  };
}
