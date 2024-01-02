#include <Retina/Platform/Input.hpp>
#include <Retina/Platform/PlatformManager.hpp>
#include <Retina/Platform/Window.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <ranges>

namespace Retina {
    constexpr static auto KEYBOARD_KEYS = std::to_array({
        EKeyboard::E_SPACE,
        EKeyboard::E_APOSTROPHE,
        EKeyboard::E_COMMA,
        EKeyboard::E_MINUS,
        EKeyboard::E_PERIOD,
        EKeyboard::E_SLASH,
        EKeyboard::E_0,
        EKeyboard::E_1,
        EKeyboard::E_2,
        EKeyboard::E_3,
        EKeyboard::E_4,
        EKeyboard::E_5,
        EKeyboard::E_6,
        EKeyboard::E_7,
        EKeyboard::E_8,
        EKeyboard::E_9,
        EKeyboard::E_SEMICOLON,
        EKeyboard::E_EQUAL,
        EKeyboard::E_A,
        EKeyboard::E_B,
        EKeyboard::E_C,
        EKeyboard::E_D,
        EKeyboard::E_E,
        EKeyboard::E_F,
        EKeyboard::E_G,
        EKeyboard::E_H,
        EKeyboard::E_I,
        EKeyboard::E_J,
        EKeyboard::E_K,
        EKeyboard::E_L,
        EKeyboard::E_M,
        EKeyboard::E_N,
        EKeyboard::E_O,
        EKeyboard::E_P,
        EKeyboard::E_Q,
        EKeyboard::E_R,
        EKeyboard::E_S,
        EKeyboard::E_T,
        EKeyboard::E_U,
        EKeyboard::E_V,
        EKeyboard::E_W,
        EKeyboard::E_X,
        EKeyboard::E_Y,
        EKeyboard::E_Z,
        EKeyboard::E_LEFT_BRACKET,
        EKeyboard::E_BACKSLASH,
        EKeyboard::E_RIGHT_BRACKET,
        EKeyboard::E_GRAVE_ACCENT,
        EKeyboard::E_WORLD_1,
        EKeyboard::E_WORLD_2,
        EKeyboard::E_ESCAPE,
        EKeyboard::E_ENTER,
        EKeyboard::E_TAB,
        EKeyboard::E_BACKSPACE,
        EKeyboard::E_INSERT,
        EKeyboard::E_DELETE,
        EKeyboard::E_RIGHT,
        EKeyboard::E_LEFT,
        EKeyboard::E_DOWN,
        EKeyboard::E_UP,
        EKeyboard::E_PAGE_UP,
        EKeyboard::E_PAGE_DOWN,
        EKeyboard::E_HOME,
        EKeyboard::E_END,
        EKeyboard::E_CAPS_LOCK,
        EKeyboard::E_SCROLL_LOCK,
        EKeyboard::E_NUM_LOCK,
        EKeyboard::E_PRINT_SCREEN,
        EKeyboard::E_PAUSE,
        EKeyboard::E_F1,
        EKeyboard::E_F2,
        EKeyboard::E_F3,
        EKeyboard::E_F4,
        EKeyboard::E_F5,
        EKeyboard::E_F6,
        EKeyboard::E_F7,
        EKeyboard::E_F8,
        EKeyboard::E_F9,
        EKeyboard::E_F10,
        EKeyboard::E_F11,
        EKeyboard::E_F12,
        EKeyboard::E_F13,
        EKeyboard::E_F14,
        EKeyboard::E_F15,
        EKeyboard::E_F16,
        EKeyboard::E_F17,
        EKeyboard::E_F18,
        EKeyboard::E_F19,
        EKeyboard::E_F20,
        EKeyboard::E_F21,
        EKeyboard::E_F22,
        EKeyboard::E_F23,
        EKeyboard::E_F24,
        EKeyboard::E_F25,
        EKeyboard::E_KP_0,
        EKeyboard::E_KP_1,
        EKeyboard::E_KP_2,
        EKeyboard::E_KP_3,
        EKeyboard::E_KP_4,
        EKeyboard::E_KP_5,
        EKeyboard::E_KP_6,
        EKeyboard::E_KP_7,
        EKeyboard::E_KP_8,
        EKeyboard::E_KP_9,
        EKeyboard::E_KP_DECIMAL,
        EKeyboard::E_KP_DIVIDE,
        EKeyboard::E_KP_MULTIPLY,
        EKeyboard::E_KP_SUBTRACT,
        EKeyboard::E_KP_ADD,
        EKeyboard::E_KP_ENTER,
        EKeyboard::E_KP_EQUAL,
        EKeyboard::E_LEFT_SHIFT,
        EKeyboard::E_LEFT_CONTROL,
        EKeyboard::E_LEFT_ALT,
        EKeyboard::E_LEFT_SUPER,
        EKeyboard::E_RIGHT_SHIFT,
        EKeyboard::E_RIGHT_CONTROL,
        EKeyboard::E_RIGHT_ALT,
        EKeyboard::E_RIGHT_SUPER,
        EKeyboard::E_MENU,
    });

    constexpr static auto KEY_RELEASED_MASK = 0x1_u32;
    constexpr static auto KEY_PRESSED_MASK = 0x2_u32;
    constexpr static auto KEY_REPEATED_MASK = 0x4_u32;
    constexpr static auto KEY_RELEASED_SHIFT = 0_u32;
    constexpr static auto KEY_PRESSED_SHIFT = 1_u32;
    constexpr static auto KEY_REPEATED_SHIFT = 2_u32;

    CInput::CInput(const CWindow& window) noexcept
        : _window(window) {
        RETINA_PROFILE_SCOPED();
    }


    auto CInput::Make(const CWindow& window) noexcept -> Self {
        RETINA_PROFILE_SCOPED();
        return { window };
    }

    auto CInput::GetWindow() const noexcept -> const CWindow& {
        RETINA_PROFILE_SCOPED();
        return _window;
    }

    auto CInput::Update() noexcept -> void {
        RETINA_PROFILE_SCOPED();
        const auto& window = GetWindow();
        std::memcpy(_previousKeyStates.data(), _keyStates.data(), _keyStates.size());
        std::memset(_keyStates.data(), 0, _keyStates.size());
        for (const auto key : KEYBOARD_KEYS) {
            _keyStates[ToUnderlying(key)] |= Platform::IsKeyReleased(window.GetHandle(), key) << KEY_RELEASED_SHIFT;
            _keyStates[ToUnderlying(key)] |= Platform::IsKeyPressed(window.GetHandle(), key) << KEY_PRESSED_SHIFT;
            _keyStates[ToUnderlying(key)] |= Platform::IsKeyRepeated(window.GetHandle(), key) << KEY_REPEATED_SHIFT;
        }
    }

    auto CInput::IsKeyReleased(EKeyboard key) const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        return _keyStates[ToUnderlying(key)] & KEY_RELEASED_MASK;
    }

    auto CInput::IsKeyPressed(EKeyboard key) const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        return _keyStates[ToUnderlying(key)] & KEY_PRESSED_MASK;
    }

    auto CInput::IsKeyRepeated(EKeyboard key) const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        return _keyStates[ToUnderlying(key)] & KEY_REPEATED_MASK;
    }

    auto CInput::IsKeyReleasedOnce(EKeyboard key) const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        return IsKeyReleased(key) && !(_previousKeyStates[ToUnderlying(key)] & KEY_RELEASED_MASK);
    }

    auto CInput::IsKeyPressedOnce(EKeyboard key) const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        return IsKeyPressed(key) && !(_previousKeyStates[ToUnderlying(key)] & KEY_PRESSED_MASK);
    }

    auto CInput::IsKeyRepeatedOnce(EKeyboard key) const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        return IsKeyRepeated(key) && !(_previousKeyStates[ToUnderlying(key)] & KEY_REPEATED_MASK);
    }

    auto CInput::IsAnyKeyReleased() const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        return std::ranges::any_of(KEYBOARD_KEYS, [this](const auto key) { return IsKeyReleased(key); });
    }

    auto CInput::IsAnyKeyPressed() const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        return std::ranges::any_of(KEYBOARD_KEYS, [this](const auto key) { return IsKeyPressed(key); });
    }

    auto CInput::IsAnyKeyRepeated() const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        return std::ranges::any_of(KEYBOARD_KEYS, [this](const auto key) { return IsKeyRepeated(key); });
    }
}
