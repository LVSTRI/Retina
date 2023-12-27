#include <Retina/Graphics/Instance.hpp>

#include <Retina/Platform/PlatformManager.hpp>
#include <Retina/Platform/WindowInfo.hpp>
#include <Retina/Platform/Window.hpp>

#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <array>

namespace Retina::Platform {
    namespace Private {
        const static class CPlatformManager {
        public:
            CPlatformManager() noexcept {
                RETINA_PROFILE_SCOPED();
                auto logger = spdlog::stdout_color_mt("PlatformManager");
                RETINA_LOG_INFO(*logger, "Initializing PlatformManager");
                RETINA_ASSERT_WITH(glfwInit(), "Failed to initialize PlatformManager");
                _logger = std::move(logger);
            }

            ~CPlatformManager() noexcept {
                RETINA_PROFILE_SCOPED();
                RETINA_LOG_INFO(*_logger, "Terminating PlatformManager");
                glfwTerminate();
            }

            RETINA_NODISCARD auto GetLogger() const noexcept -> spdlog::logger& {
                return *_logger;
            }

        private:
            std::shared_ptr<spdlog::logger> _logger;
        } platformManager = {};
    }

    auto MakeWindowHandle(std::string_view title, uint32 width, uint32 height, const SWindowFeatures& features) noexcept -> NativeHandle {
        RETINA_PROFILE_SCOPED();
        const auto featureHintMapping = std::to_array({
            std::make_pair(GLFW_RESIZABLE, features.Resizable),
            std::make_pair(GLFW_DECORATED, features.Decorated),
            std::make_pair(GLFW_FOCUSED, features.Focused),
        });
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        for (const auto& [hint, value] : featureHintMapping) {
            glfwWindowHint(hint, value);
        }
        auto* monitor = glfwGetPrimaryMonitor();
        const auto* videoMode = glfwGetVideoMode(monitor);
        const auto maxWidth = videoMode->width;
        const auto maxHeight = videoMode->height;
        auto position_x = 0_i32;
        auto position_y = 0_i32;
        glfwGetMonitorPos(monitor, &position_x, &position_y);
        const auto x = static_cast<float32>(maxWidth) / 2.0f - static_cast<float32>(width) / 2.0f + static_cast<float32>(position_x);
        const auto y = static_cast<float32>(maxHeight) / 2.0f - static_cast<float32>(height) / 2.0f + static_cast<float32>(position_y);
        auto* windowHandle = glfwCreateWindow(static_cast<int32>(width), static_cast<int32>(height), title.data(), nullptr, nullptr);
        RETINA_ASSERT_WITH(windowHandle, "Failed to create window");
        glfwSetWindowPos(windowHandle, static_cast<int32>(x), static_cast<int32>(y));
        return windowHandle;
    }

    auto IsWindowOpen(NativeHandle handle) noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        return !glfwWindowShouldClose(static_cast<GLFWwindow*>(handle));
    }

    auto GetWindowViewport(NativeHandle handle) noexcept -> std::pair<uint32, uint32> {
        RETINA_PROFILE_SCOPED();
        auto width = 0_i32;
        auto height = 0_i32;
        glfwGetFramebufferSize(static_cast<GLFWwindow*>(handle), &width, &height);
        return std::make_pair(width, height);
    }

    auto DestroyWindowHandle(NativeHandle handle) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        glfwDestroyWindow(static_cast<GLFWwindow*>(handle));
    }

    auto GetSurfaceExtensions() noexcept -> std::span<const char*> {
        RETINA_PROFILE_SCOPED();
        auto count = 0_u32;
        const auto extensions = glfwGetRequiredInstanceExtensions(&count);
        return { extensions, count };
    }

    auto MakeNativeSurface(const CInstance& instance, const CWindow& window) noexcept -> SurfaceHandle {
        RETINA_PROFILE_SCOPED();
        auto surface = VkSurfaceKHR();
        RETINA_VULKAN_CHECK(
            instance.GetLogger(),
            glfwCreateWindowSurface(
                instance.GetHandle(),
                static_cast<GLFWwindow*>(window.GetHandle()),
                nullptr,
                &surface
            )
        );
        return surface;
    }

    auto PollEvents() noexcept -> void {
        RETINA_PROFILE_SCOPED();
        glfwPollEvents();
    }

    auto WaitEvents() noexcept -> void {
        RETINA_PROFILE_SCOPED();
        glfwWaitEvents();
    }
}
