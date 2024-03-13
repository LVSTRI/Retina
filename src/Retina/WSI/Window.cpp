#include <Retina/WSI/Logger.hpp>
#include <Retina/WSI/Window.hpp>

#include <GLFW/glfw3.h>

#include <array>

namespace Retina::WSI {
  namespace Details {
    auto MakePlatformWindow(const SWindowCreateInfo& createInfo) noexcept -> GLFWwindow* {
      RETINA_PROFILE_SCOPED();
      auto featureHints = std::to_array({
        std::make_pair(GLFW_RESIZABLE, createInfo.Features.Resizable),
        std::make_pair(GLFW_DECORATED, createInfo.Features.Decorated),
        std::make_pair(GLFW_FOCUSED, createInfo.Features.Focused)
      });
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      for (const auto& [hint, value] : featureHints) {
        glfwWindowHint(hint, value);
      }

      auto* primaryMonitor = glfwGetPrimaryMonitor();
      const auto videoMode = glfwGetVideoMode(primaryMonitor);
      const auto maxMonitorWidth = videoMode->width;
      const auto maxMonitorHeight = videoMode->height;

      auto monitorX = 0_i32;
      auto monitorY = 0_i32;
      glfwGetMonitorPos(primaryMonitor, &monitorX, &monitorY);

      const auto windowX =
        static_cast<float32>(maxMonitorWidth) / 2.0f -
        static_cast<float32>(createInfo.Width) / 2.0f +
        static_cast<float32>(monitorX);
      const auto windowY =
        static_cast<float32>(maxMonitorHeight) / 2.0f -
        static_cast<float32>(createInfo.Height) / 2.0f +
        static_cast<float32>(monitorY);

      auto* windowHandle = glfwCreateWindow(createInfo.Width, createInfo.Height, createInfo.Title.c_str(), nullptr, nullptr);
      RETINA_ASSERT_WITH(windowHandle, "Failed to create window");
      glfwSetWindowPos(windowHandle, static_cast<int32>(windowX), static_cast<int32>(windowY));
      return windowHandle;
    }
  }

  CWindow::~CWindow() noexcept {
      RETINA_PROFILE_SCOPED();
    if (_handle) {
      glfwDestroyWindow(static_cast<GLFWwindow*>(_handle));
      RETINA_WSI_INFO("Window ({}) destroyed", _createInfo.Title);
    }
  }

  CWindow::CWindow(CWindow&& other) noexcept
    : _handle(std::exchange(other._handle, {})),
      _dispatcher(std::exchange(other._dispatcher, {})),
      _createInfo(std::exchange(other._createInfo, {}))
  {
    RETINA_PROFILE_SCOPED();
  }

  auto CWindow::operator =(CWindow&& other) noexcept -> CWindow& {
    RETINA_PROFILE_SCOPED();
    if (this == &other) {
      return *this;
    }
    return Core::Reconstruct(*this, std::move(other));
  }

  auto CWindow::Make(const SWindowCreateInfo& createInfo) noexcept -> std::unique_ptr<CWindow> {
    RETINA_PROFILE_SCOPED();
    auto self = std::make_unique<CWindow>();
    auto dispatcher = EventDispatcher::Make();
    auto* windowHandle = Details::MakePlatformWindow(createInfo);
    glfwSetWindowUserPointer(windowHandle, self.get());
    glfwSetWindowSizeCallback(windowHandle, [](GLFWwindow* window, int32 width, int32 height) {
      auto& self = *static_cast<CWindow*>(glfwGetWindowUserPointer(window));
      self._createInfo.Width = width;
      self._createInfo.Height = height;
      self.GetEventDispatcher().Fire<SWindowResizeEvent>(width, height);
    });
    glfwSetWindowCloseCallback(windowHandle, [](GLFWwindow* window) {
      auto& self = *static_cast<CWindow*>(glfwGetWindowUserPointer(window));
      self.GetEventDispatcher().Fire<SWindowCloseEvent>();
    });
    glfwSetKeyCallback(windowHandle, [](GLFWwindow* window, int32 key, int32 scancode, int32 action, int32 modifiers) {
      auto& self = *static_cast<CWindow*>(glfwGetWindowUserPointer(window));
      self.GetEventDispatcher().Fire<SWindowKeyboardEvent>(
        static_cast<EInputKeyboard>(key),
        static_cast<EInputAction>(action),
        static_cast<EInputModifier>(modifiers),
        scancode
      );
    });
    glfwSetMouseButtonCallback(windowHandle, [](GLFWwindow* window, int32 button, int32 action, int32 modifiers) {
      auto& self = *static_cast<CWindow*>(glfwGetWindowUserPointer(window));
      self.GetEventDispatcher().Fire<SWindowMouseButtonEvent>(
        static_cast<EInputMouse>(button),
        static_cast<EInputAction>(action),
        static_cast<EInputModifier>(modifiers)
      );
    });
    glfwSetCursorPosCallback(windowHandle, [](GLFWwindow* window, float64 x, float64 y) {
      auto& self = *static_cast<CWindow*>(glfwGetWindowUserPointer(window));
      self.GetEventDispatcher().Fire<SWindowMousePositionEvent>(
        static_cast<float32>(x),
        static_cast<float32>(y)
      );
    });
    glfwSetScrollCallback(windowHandle, [](GLFWwindow* window, float64 x, float64 y) {
      auto& self = *static_cast<CWindow*>(glfwGetWindowUserPointer(window));
      self.GetEventDispatcher().Fire<SWindowMouseScrollEvent>(
        static_cast<float32>(x),
        static_cast<float32>(y)
      );
    });
    self->_handle = windowHandle;
    self->_dispatcher = std::move(dispatcher);
    self->_createInfo = createInfo;

    RETINA_WSI_INFO("Initialized Window: {{ {}, {}, {} }}", createInfo.Title, createInfo.Width, createInfo.Height);
    RETINA_WSI_INFO("- Resizable: {}", createInfo.Features.Resizable);
    RETINA_WSI_INFO("- Decorated: {}", createInfo.Features.Decorated);
    RETINA_WSI_INFO("- Focused: {}", createInfo.Features.Focused);

    return self;
  }

  auto CWindow::GetHandle() const noexcept -> WindowHandle {
    RETINA_PROFILE_SCOPED();
    return _handle;
  }

  auto CWindow::GetEventDispatcher() noexcept -> EventDispatcher& {
    RETINA_PROFILE_SCOPED();
    return _dispatcher;
  }

  auto CWindow::GetCreateInfo() const noexcept -> const SWindowCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CWindow::GetTitle() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Title;
  }

  auto CWindow::GetWidth() const noexcept -> uint32 {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Width;
  }

  auto CWindow::GetHeight() const noexcept -> uint32 {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Height;
  }

  auto CWindow::IsFeatureEnabled(bool SWindowFeature::* feature) const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Features.*feature;
  }

  auto CWindow::IsOpen() const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return !glfwWindowShouldClose(static_cast<GLFWwindow*>(_handle));
  }
}
