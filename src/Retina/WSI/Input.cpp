#include <Retina/WSI/Input.hpp>
#include <Retina/WSI/Window.hpp>

#include <GLFW/glfw3.h>

namespace Retina::WSI {
  CInput::CInput(CWindow& window) noexcept
    : _window(window)
  {
    RETINA_PROFILE_SCOPED();
  }

  CInput::CInput(CInput&& other) noexcept
    : _dispatcher(std::exchange(other._dispatcher, {})),
      _window(std::exchange(other._window, {}))
  {
    RETINA_PROFILE_SCOPED();
  }

  auto CInput::operator =(CInput&& other) noexcept -> CInput& {
    RETINA_PROFILE_SCOPED();
    if (this == &other) {
      return *this;
    }
    return Core::Reconstruct(*this, std::move(other));
  }

  auto CInput::Make(CWindow& window) noexcept -> CInput {
    RETINA_PROFILE_SCOPED();
    auto self = CInput(window);
    self._dispatcher = EventDispatcher::Make();
    return self;
  }

  auto CInput::GetEventDispatcher() noexcept -> EventDispatcher& {
    RETINA_PROFILE_SCOPED();
    return _dispatcher;
  }

  auto CInput::IsKeyPressed(EInputKeyboard key) const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return glfwGetKey(
      static_cast<GLFWwindow*>(_window->GetHandle()),
      static_cast<int32>(key)
    ) == GLFW_PRESS;
  }

  auto CInput::IsKeyReleased(EInputKeyboard key) const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return glfwGetKey(
      static_cast<GLFWwindow*>(_window->GetHandle()),
      static_cast<int32>(key)
    ) == GLFW_RELEASE;
  }

  auto CInput::IsKeyRepeated(EInputKeyboard key) const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return glfwGetKey(
      static_cast<GLFWwindow*>(_window->GetHandle()),
      static_cast<int32>(key)
    ) == GLFW_REPEAT;
  }

  auto CInput::IsMouseButtonPressed(EInputMouse button) const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return glfwGetMouseButton(
      static_cast<GLFWwindow*>(_window->GetHandle()),
      static_cast<int32>(button)
    ) == GLFW_PRESS;
  }

  auto CInput::IsMouseButtonReleased(EInputMouse button) const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return glfwGetMouseButton(
      static_cast<GLFWwindow*>(_window->GetHandle()),
      static_cast<int32>(button)
    ) == GLFW_RELEASE;
  }

  auto CInput::IsMouseButtonRepeated(EInputMouse button) const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return glfwGetMouseButton(
      static_cast<GLFWwindow*>(_window->GetHandle()),
      static_cast<int32>(button)
    ) == GLFW_REPEAT;
  }

  auto CInput::GetCursorPosition() const noexcept -> SInputCursorPosition {
    RETINA_PROFILE_SCOPED();
    double x, y;
    glfwGetCursorPos(
      static_cast<GLFWwindow*>(_window->GetHandle()),
      &x,
      &y
    );
    return { x, y };
  }

  auto CInput::SetCursorMode(EInputCursorMode mode) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    glfwSetInputMode(
      static_cast<GLFWwindow*>(_window->GetHandle()),
      GLFW_CURSOR,
      static_cast<int32>(mode)
    );
    if (mode == EInputCursorMode::E_DISABLED) {
      if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(static_cast<GLFWwindow*>(_window->GetHandle()), GLFW_RAW_MOUSE_MOTION, true);
      }
    }
    _dispatcher.Fire<SInputCursorModeEvent>(mode);
  }
}
