#include <Retina/Sandbox/Camera.hpp>

#include <Retina/WSI/Input.hpp>
#include <Retina/WSI/WindowInfo.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/reciprocal.hpp>

namespace Retina::Sandbox {
  CCamera::CCamera(WSI::CInput& input) noexcept
    : _input(input)
  {
    RETINA_PROFILE_SCOPED();
    _input->GetEventDispatcher().Attach(this, &CCamera::OnInputCursorMode);
  }

  auto CCamera::Make(WSI::CInput& input) noexcept -> std::unique_ptr<CCamera> {
    RETINA_PROFILE_SCOPED();
    return std::make_unique<CCamera>(input);
  }

  auto CCamera::GetPosition() const noexcept -> const glm::vec3& {
    RETINA_PROFILE_SCOPED();
    return _position;
  }

  auto CCamera::GetFront() const noexcept -> const glm::vec3& {
    RETINA_PROFILE_SCOPED();
    return _front;
  }

  auto CCamera::GetUp() const noexcept -> const glm::vec3& {
    RETINA_PROFILE_SCOPED();
    return _up;
  }

  auto CCamera::GetRight() const noexcept -> const glm::vec3& {
    RETINA_PROFILE_SCOPED();
    return _right;
  }

  auto CCamera::GetYaw() const noexcept -> float32 {
    RETINA_PROFILE_SCOPED();
    return _yaw;
  }

  auto CCamera::GetPitch() const noexcept -> float32 {
    RETINA_PROFILE_SCOPED();
    return _pitch;
  }

  auto CCamera::GetViewSensitivity() const noexcept -> float32 {
    RETINA_PROFILE_SCOPED();
    return _viewSensitivity;
  }

  auto CCamera::GetMovementSpeed() const noexcept -> float32 {
    RETINA_PROFILE_SCOPED();
    return _movementSpeed;
  }

  auto CCamera::SetViewSensitivity(float32 sensitivity) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _viewSensitivity = sensitivity;
  }

  auto CCamera::SetMovementSpeed(float32 speed) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _movementSpeed = speed;
  }

  auto CCamera::GetInput() const noexcept -> const WSI::CInput& {
    RETINA_PROFILE_SCOPED();
    return *_input;
  }

  auto CCamera::Update(float32 deltaTime) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    const auto dtMovementSpeed = _movementSpeed * deltaTime;

    const auto [cursorX, cursorY] = _input->GetCursorPosition();
    _lastCursorPosition = _cursorPosition;
    _cursorPosition.X = static_cast<float32>(cursorX);
    _cursorPosition.Y = static_cast<float32>(cursorY);

    const auto cursorDeltaX = _cursorPosition.X - _lastCursorPosition.X;
    const auto cursorDeltaY = _lastCursorPosition.Y - _cursorPosition.Y;

    if (_isCaptured) {
      _yaw += cursorDeltaX * _viewSensitivity;
      _pitch += cursorDeltaY * _viewSensitivity;
    }
    _pitch = glm::clamp(_pitch, -89.9f, 89.9f);

    const auto radYaw = glm::radians(_yaw);
    const auto radPitch = glm::radians(_pitch);

    if (_input->IsKeyPressed(WSI::EInputKeyboard::E_W)) {
      _position.x += glm::cos(radYaw) * dtMovementSpeed;
      _position.z += glm::sin(radYaw) * dtMovementSpeed;
    }
    if (_input->IsKeyPressed(WSI::EInputKeyboard::E_S)) {
      _position.x -= glm::cos(radYaw) * dtMovementSpeed;
      _position.z -= glm::sin(radYaw) * dtMovementSpeed;
    }
    if (_input->IsKeyPressed(WSI::EInputKeyboard::E_A)) {
      _position -= _right * dtMovementSpeed;
    }
    if (_input->IsKeyPressed(WSI::EInputKeyboard::E_D)) {
      _position += _right * dtMovementSpeed;
    }
    if (_input->IsKeyPressed(WSI::EInputKeyboard::E_SPACE)) {
      _position.y += dtMovementSpeed;
    }
    if (_input->IsKeyPressed(WSI::EInputKeyboard::E_LEFT_SHIFT)) {
      _position.y -= dtMovementSpeed;
    }

    _front = glm::normalize(
      glm::vec3(
        glm::cos(radPitch) * glm::cos(radYaw),
        glm::sin(radPitch),
        glm::cos(radPitch) * glm::sin(radYaw)
      )
    );
    _right = glm::normalize(glm::cross(_front, glm::vec3(0.0f, 1.0f, 0.0f)));
    _up = glm::normalize(glm::cross(_right, _front));
  }

  auto CCamera::GetViewMatrix() const noexcept -> glm::mat4 {
    RETINA_PROFILE_SCOPED();
    return glm::lookAt(_position, _position + _front, _up);
  }

  auto CCamera::OnInputCursorMode(const WSI::SInputCursorModeEvent& event) noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    const auto [cursorX, cursorY] = _input->GetCursorPosition();
    _lastCursorPosition.X = static_cast<float32>(cursorX);
    _lastCursorPosition.Y = static_cast<float32>(cursorY);
    _cursorPosition = _lastCursorPosition;
    _isCaptured = event.Mode == WSI::EInputCursorMode::E_DISABLED;
    return false;
  }

  auto MakeInfiniteReversePerspective(float32 fov, float32 aspect, float32 near) noexcept -> glm::mat4 {
    RETINA_PROFILE_SCOPED();
    const auto cotHalfFov = glm::cot(glm::radians(fov) / 2.0f);
    auto projection = glm::mat4(
      cotHalfFov / aspect, 0.0f, 0.0f, 0.0f,
      0.0f, cotHalfFov, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, -1.0f,
      0.0f, 0.0f, near, 0.0f
    );
    projection[1][1] *= -1;
    return projection;
  }
}
