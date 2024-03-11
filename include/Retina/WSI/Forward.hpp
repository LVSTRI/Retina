#pragma once

namespace Retina::WSI {
  class CWindow;

  struct SWindowFeature;
  struct SWindowCreateInfo;
  enum class EInputModifier;
  enum class EInputAction;
  enum class EInputMouse;
  enum class EInputKeyboard;
  struct SWindowResizeEvent;
  struct SWindowCloseEvent;
  struct SWindowKeyboardEvent;
  struct SWindowMouseButtonEvent;
  struct SWindowMousePositionEvent;
  struct SWindowMouseScrollEvent;

  using WindowHandle = void*;
  using SurfaceHandle = void*;
  using InstanceHandle = void*;
}
