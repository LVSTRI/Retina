#pragma once

namespace Retina::WSI {
  // <Retina/WSI/Input.hpp>
  class CInput;

  // <Retina/WSI/Window.hpp>
  class CWindow;

  // <Retina/WSI/WindowInfo.hpp>
  struct SWindowFeature;
  struct SWindowCreateInfo;
  enum class EInputModifier;
  enum class EInputAction;
  enum class EInputMouse;
  enum class EInputKeyboard;
  enum class EInputCursorMode;
  struct SWindowFocusEvent;
  struct SWindowResizeEvent;
  struct SWindowCloseEvent;
  struct SWindowKeyboardEvent;
  struct SWindowMouseButtonEvent;
  struct SWindowMousePositionEvent;
  struct SWindowMouseEnterEvent;
  struct SWindowMouseScrollEvent;
  struct SWindowInputCharEvent;
  struct SWindowMonitorEvent;
  struct SInputCursorModeEvent;
  struct SInputCursorPosition;

  using WindowHandle = void*;
  using SurfaceHandle = void*;
  using InstanceHandle = void*;

}
