#pragma once

#include <Retina/Core/Core.hpp>

#include <string>

namespace Retina::WSI {
  struct SWindowFeature {
    bool Resizable = false;
    bool Decorated = true;
    bool Focused = true;
  };

  struct SWindowCreateInfo {
    std::string Title;
    int32 Width = 0;
    int32 Height = 0;
    SWindowFeature Features = {};
  };

  enum class EInputModifier {
    E_SHIFT = 1 << 0,
    E_CONTROL = 1 << 1,
    E_ALT = 1 << 2,
    E_SUPER = 1 << 3,
    E_CAPS_LOCK = 1 << 4,
    E_NUM_LOCK = 1 << 5,
  };

  enum class EInputAction {
    E_RELEASE = 0,
    E_PRESS = 1,
    E_REPEAT = 2,
  };

  enum class EInputMouse {
    E_BUTTON_1 = 0,
    E_BUTTON_2 = 1,
    E_BUTTON_3 = 2,
    E_BUTTON_4 = 3,
    E_BUTTON_5 = 4,
    E_BUTTON_6 = 5,
    E_BUTTON_7 = 6,
    E_BUTTON_8 = 7,
    E_BUTTON_LAST = E_BUTTON_8,
    E_BUTTON_LEFT = E_BUTTON_1,
    E_BUTTON_RIGHT = E_BUTTON_2,
    E_BUTTON_MIDDLE = E_BUTTON_3,
  };

  enum class EInputKeyboard {
    E_UNKNOWN = -1,
    E_SPACE = 32,
    E_APOSTROPHE = 39,
    E_COMMA = 44,
    E_MINUS = 45,
    E_PERIOD = 46,
    E_SLASH = 47,
    E_0 = 48,
    E_1 = 49,
    E_2 = 50,
    E_3 = 51,
    E_4 = 52,
    E_5 = 53,
    E_6 = 54,
    E_7 = 55,
    E_8 = 56,
    E_9 = 57,
    E_SEMICOLON = 59,
    E_EQUAL = 61,
    E_A = 65,
    E_B = 66,
    E_C = 67,
    E_D = 68,
    E_E = 69,
    E_F = 70,
    E_G = 71,
    E_H = 72,
    E_I = 73,
    E_J = 74,
    E_K = 75,
    E_L = 76,
    E_M = 77,
    E_N = 78,
    E_O = 79,
    E_P = 80,
    E_Q = 81,
    E_R = 82,
    E_S = 83,
    E_T = 84,
    E_U = 85,
    E_V = 86,
    E_W = 87,
    E_X = 88,
    E_Y = 89,
    E_Z = 90,
    E_LEFT_BRACKET = 91,
    E_BACKSLASH = 92,
    E_RIGHT_BRACKET = 93,
    E_GRAVE_ACCENT = 96,
    E_WORLD_1 = 161,
    E_WORLD_2 = 162,
    E_ESCAPE = 256,
    E_ENTER = 257,
    E_TAB = 258,
    E_BACKSPACE = 259,
    E_INSERT = 260,
    E_DELETE = 261,
    E_RIGHT = 262,
    E_LEFT = 263,
    E_DOWN = 264,
    E_UP = 265,
    E_PAGE_UP = 266,
    E_PAGE_DOWN = 267,
    E_HOME = 268,
    E_END = 269,
    E_CAPS_LOCK = 280,
    E_SCROLL_LOCK = 281,
    E_NUM_LOCK = 282,
    E_PRINT_SCREEN = 283,
    E_PAUSE = 284,
    E_F1 = 290,
    E_F2 = 291,
    E_F3 = 292,
    E_F4 = 293,
    E_F5 = 294,
    E_F6 = 295,
    E_F7 = 296,
    E_F8 = 297,
    E_F9 = 298,
    E_F10 = 299,
    E_F11 = 300,
    E_F12 = 301,
    E_F13 = 302,
    E_F14 = 303,
    E_F15 = 304,
    E_F16 = 305,
    E_F17 = 306,
    E_F18 = 307,
    E_F19 = 308,
    E_F20 = 309,
    E_F21 = 310,
    E_F22 = 311,
    E_F23 = 312,
    E_F24 = 313,
    E_F25 = 314,
    E_KP_0 = 320,
    E_KP_1 = 321,
    E_KP_2 = 322,
    E_KP_3 = 323,
    E_KP_4 = 324,
    E_KP_5 = 325,
    E_KP_6 = 326,
    E_KP_7 = 327,
    E_KP_8 = 328,
    E_KP_9 = 329,
    E_KP_DECIMAL = 330,
    E_KP_DIVIDE = 331,
    E_KP_MULTIPLY = 332,
    E_KP_SUBTRACT = 333,
    E_KP_ADD = 334,
    E_KP_ENTER = 335,
    E_KP_EQUAL = 336,
    E_LEFT_SHIFT = 340,
    E_LEFT_CONTROL = 341,
    E_LEFT_ALT = 342,
    E_LEFT_SUPER = 343,
    E_RIGHT_SHIFT = 344,
    E_RIGHT_CONTROL = 345,
    E_RIGHT_ALT = 346,
    E_RIGHT_SUPER = 347,
    E_MENU = 348,
  };

  enum class EInputCursorMode {
    E_NORMAL = 0x00034001,
    E_HIDDEN = 0x00034002,
    E_DISABLED = 0x00034003,
  };

  struct SWindowResizeEvent {
    int32 Width = 0;
    int32 Height = 0;
  };

  struct SWindowCloseEvent {};

  struct SWindowKeyboardEvent {
    EInputKeyboard Key = EInputKeyboard::E_UNKNOWN;
    EInputAction Action = EInputAction::E_RELEASE;
    EInputModifier Modifiers = {};
    int32 Scancode = 0;
  };

  struct SWindowMouseButtonEvent {
    EInputMouse Button = EInputMouse::E_BUTTON_1;
    EInputAction Action = EInputAction::E_RELEASE;
    EInputModifier Modifiers = {};
  };

  struct SWindowMousePositionEvent {
    float64 X = 0.0;
    float64 Y = 0.0;
  };

  struct SWindowMouseScrollEvent {
    float64 X = 0.0;
    float64 Y = 0.0;
  };

  struct SInputCursorModeEvent {
    EInputCursorMode Mode = {};
  };

  struct SInputCursorPosition {
    float64 X = 0.0;
    float64 Y = 0.0;
  };
}
