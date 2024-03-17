#include <Retina/WSI/Logger.hpp>
#include <Retina/WSI/Platform.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Retina::WSI {
  auto Initialize() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_ASSERT_WITH(glfwInit(), "Failed to initialize GLFW");
    RETINA_WSI_INFO("Initialized WSI Backend");
    RETINA_STATIC_DEFER([] {
      RETINA_PROFILE_SCOPED();
      glfwTerminate();
      RETINA_WSI_INFO("Terminated WSI Backend");
    });
  }

  auto PollEvents() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    glfwPollEvents();
  }

  auto WaitEvents() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    glfwWaitEvents();
  }

  auto GetKeyName(EInputKeyboard key, int32 scancode) noexcept -> const char* {
    RETINA_PROFILE_SCOPED();
    return glfwGetKeyName(std::to_underlying(key), scancode);
  }

  auto GetSurfaceExtensionNames() noexcept -> std::span<const char*> {
    RETINA_PROFILE_SCOPED();
    auto count = 0_u32;
    const auto** extensions = glfwGetRequiredInstanceExtensions(&count);
    return { extensions, count };
  }

  auto MakeSurface(InstanceHandle instance, WindowHandle window) noexcept -> SurfaceHandle {
    RETINA_PROFILE_SCOPED();
    auto surface = VkSurfaceKHR();
    RETINA_ASSERT_WITH(
      glfwCreateWindowSurface(
        static_cast<VkInstance>(instance),
        static_cast<GLFWwindow*>(window),
        nullptr,
        &surface
      ) == VK_SUCCESS,
      "Failed to create window surface"
    );
    return surface;
  }
}
