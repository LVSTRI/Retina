#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/QueueInfo.hpp>
#include <Retina/Graphics/Native/NativeDebugName.hpp>

#include <vulkan/vulkan.h>

#include <spdlog/spdlog.h>

#include <mutex>

namespace Retina {
    class CQueue : public INativeDebugName, public IEnableIntrusiveReferenceCount<CQueue> {
    public:
        using Self = CQueue;

        CQueue(const CDevice& device) noexcept;
        ~CQueue() noexcept;

        RETINA_NODISCARD static auto Make(const CDevice& device, const SQueueCreateInfo& createInfo) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkQueue;
        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SQueueCreateInfo&;
        RETINA_NODISCARD auto GetLogger() const noexcept -> spdlog::logger&;
        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

        RETINA_NODISCARD auto GetFamilyInfo() const noexcept -> const SQueueFamilyInfo&;
        RETINA_NODISCARD auto GetFamilyIndex() const noexcept -> uint32;

        auto SetDebugName(std::string_view name) noexcept -> void;

        auto WaitIdle() noexcept -> void;
        auto Submit(const SQueueSubmitInfo& submitInfo, const CFence* fence = nullptr) noexcept -> void;
        auto Submit(std::function<void(CCommandBuffer&)>&& callback) noexcept -> void;
        auto Present(const SQueuePresentInfo& presentInfo) noexcept -> bool;

    private:
        VkQueue _handle = {};
        std::mutex _lock;

        SQueueCreateInfo _createInfo = {};
        std::shared_ptr<spdlog::logger> _logger;
        std::reference_wrapper<const CDevice> _device;
    };
}
