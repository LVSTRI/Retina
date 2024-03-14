#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/WSI/Forward.hpp>

#include <glm/glm.hpp>

#include <memory>

namespace Retina::Entry {
  class CCamera {
  private:
    struct SCursorPosition {
      float32 X = 0.0f;
      float32 Y = 0.0f;
    };

  public:
    CCamera(WSI::CInput& input) noexcept;
    ~CCamera() noexcept = default;

    RETINA_NODISCARD static auto Make(WSI::CInput& input) noexcept -> std::unique_ptr<CCamera>;

    RETINA_NODISCARD auto GetPosition() const noexcept -> const glm::vec3&;
    RETINA_NODISCARD auto GetFront() const noexcept -> const glm::vec3&;
    RETINA_NODISCARD auto GetUp() const noexcept -> const glm::vec3&;
    RETINA_NODISCARD auto GetRight() const noexcept -> const glm::vec3&;
    RETINA_NODISCARD auto GetYaw() const noexcept -> float32;
    RETINA_NODISCARD auto GetPitch() const noexcept -> float32;

    RETINA_NODISCARD auto GetViewSensitivity() const noexcept -> float32;
    RETINA_NODISCARD auto GetMovementSpeed() const noexcept -> float32;

    auto SetViewSensitivity(float32 sensitivity) noexcept -> void;
    auto SetMovementSpeed(float32 speed) noexcept -> void;

    RETINA_NODISCARD auto GetInput() const noexcept -> const WSI::CInput&;

    auto Update(float32 deltaTime) noexcept -> void;

    auto GetViewMatrix() const noexcept -> glm::mat4;

  private:
    auto OnInputCursorMode(const WSI::SInputCursorModeEvent& event) noexcept -> bool;

  private:
    glm::vec3 _position = {};
    glm::vec3 _front = {};
    glm::vec3 _up = {};
    glm::vec3 _right = {};
    float32 _yaw = 0.0f;
    float32 _pitch = 0.0f;

    float32 _viewSensitivity = 15.0f;
    float32 _movementSpeed = 5.0f;

    SCursorPosition _lastCursorPosition = {};
    SCursorPosition _cursorPosition = {};
    bool _isCaptured = false;

    Core::CReferenceWrapper<WSI::CInput> _input;
  };

  RETINA_NODISCARD auto MakeInfiniteReversePerspective(float32 fov, float32 aspect, float32 near) noexcept -> glm::mat4;
}
