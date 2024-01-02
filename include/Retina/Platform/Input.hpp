#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Platform/InputInfo.hpp>

namespace Retina {
    class CInput {
    public:
        using Self = CInput;

        CInput(const CWindow& window) noexcept;
        ~CInput() noexcept = default;

        CInput(const Self&) noexcept = delete;
        auto operator =(const Self&) noexcept -> Self& = delete;
        CInput(Self&&) noexcept = default;
        auto operator =(Self&&) noexcept -> Self& = default;

        RETINA_NODISCARD static auto Make(const CWindow& window) noexcept -> Self;

        RETINA_NODISCARD auto GetWindow() const noexcept -> const CWindow&;

        auto Update() noexcept -> void;

        RETINA_NODISCARD auto IsKeyReleased(EKeyboard key) const noexcept -> bool;
        RETINA_NODISCARD auto IsKeyPressed(EKeyboard key) const noexcept -> bool;
        RETINA_NODISCARD auto IsKeyRepeated(EKeyboard key) const noexcept -> bool;

        RETINA_NODISCARD auto IsKeyReleasedOnce(EKeyboard key) const noexcept -> bool;
        RETINA_NODISCARD auto IsKeyPressedOnce(EKeyboard key) const noexcept -> bool;
        RETINA_NODISCARD auto IsKeyRepeatedOnce(EKeyboard key) const noexcept -> bool;

        RETINA_NODISCARD auto IsAnyKeyReleased() const noexcept -> bool;
        RETINA_NODISCARD auto IsAnyKeyPressed() const noexcept -> bool;
        RETINA_NODISCARD auto IsAnyKeyRepeated() const noexcept -> bool;

    private:
        std::reference_wrapper<const CWindow> _window;

        std::array<uint8, ToUnderlying(EKeyboard::E_MENU) + 1> _keyStates = {};
        std::array<uint8, ToUnderlying(EKeyboard::E_MENU) + 1> _previousKeyStates = {};
    };
}
