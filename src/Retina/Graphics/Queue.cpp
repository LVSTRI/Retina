#include <Retina/Graphics/Sync/SyncHostTimeline.hpp>
#include <Retina/Graphics/CommandBuffer.hpp>
#include <Retina/Graphics/CommandPool.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Fence.hpp>
#include <Retina/Graphics/Queue.hpp>
#include <Retina/Graphics/Semaphore.hpp>
#include <Retina/Graphics/Swapchain.hpp>
#include <Retina/Graphics/TimelineSemaphore.hpp>

#include <spdlog/sinks/stdout_color_sinks.h>

#include <volk.h>

namespace Retina {
    CQueue::CQueue(const CDevice& device) noexcept : _device(device) {
        RETINA_PROFILE_SCOPED();
    }

    CQueue::~CQueue() noexcept {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(*_logger, "Terminating \"{}\"", GetDebugName());
    }

    auto CQueue::Make(const CDevice& device, const SQueueCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto queue = CArcPtr(new Self(device));
        auto logger = std::shared_ptr<spdlog::logger>();
        switch (createInfo.Domain) {
            case EQueueDomain::E_GRAPHICS:
                logger = spdlog::stdout_color_mt("GraphicsQueue");
                break;
            case EQueueDomain::E_COMPUTE:
                logger = spdlog::stdout_color_mt("ComputeQueue");
                break;
            case EQueueDomain::E_TRANSFER:
                logger = spdlog::stdout_color_mt("TransferQueue");
                break;
        }
        RETINA_LOG_INFO(
            *logger,
            "Creating \"{}\", Family Info: {{ {}, {} }}",
            createInfo.Name,
            createInfo.FamilyInfo.FamilyIndex,
            createInfo.FamilyInfo.QueueIndex
        );

        auto queueHandle = VkQueue();
        vkGetDeviceQueue(device.GetHandle(), createInfo.FamilyInfo.FamilyIndex, createInfo.FamilyInfo.QueueIndex, &queueHandle);

        queue->_handle = queueHandle;
        queue->_createInfo = createInfo;
        queue->_logger = std::move(logger);
        queue->SetDebugName(createInfo.Name);

        return queue;
    }

    auto CQueue::GetHandle() const noexcept -> VkQueue {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto CQueue::GetCreateInfo() const noexcept -> const SQueueCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CQueue::GetLogger() const noexcept -> spdlog::logger& {
        RETINA_PROFILE_SCOPED();
        return *_logger;
    }

    auto CQueue::GetDevice() const noexcept -> const CDevice& {
        RETINA_PROFILE_SCOPED();
        return _device;
    }

    auto CQueue::GetFamilyInfo() const noexcept -> const SQueueFamilyInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo.FamilyInfo;
    }

    auto CQueue::GetFamilyIndex() const noexcept -> uint32 {
        RETINA_PROFILE_SCOPED();
        return _createInfo.FamilyInfo.FamilyIndex;
    }

    auto CQueue::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_QUEUE;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(*_logger, vkSetDebugUtilsObjectNameEXT(GetDevice().GetHandle(), &info));
    }

    auto CQueue::WaitIdle() noexcept -> void {
        RETINA_PROFILE_SCOPED();
        auto guard = std::lock_guard(_lock);
        RETINA_VULKAN_CHECK(*_logger, vkQueueWaitIdle(_handle));
    }

    auto CQueue::Submit(const SQueueSubmitInfo& submitInfo, const CFence* fence) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        auto waitSemaphoreInfos = std::vector<VkSemaphoreSubmitInfo>();
        waitSemaphoreInfos.reserve(submitInfo.WaitSemaphores.size());
        for (const auto& [semaphore, stage, value] : submitInfo.WaitSemaphores) {
            auto semaphoreInfo = VkSemaphoreSubmitInfo(VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO);
            semaphoreInfo.semaphore = semaphore.get().GetHandle();
            semaphoreInfo.stageMask = ToEnumCounterpart(stage);
            semaphoreInfo.value = value;
            waitSemaphoreInfos.emplace_back(semaphoreInfo);
        }

        auto signalSemaphoreInfos = std::vector<VkSemaphoreSubmitInfo>();
        signalSemaphoreInfos.reserve(submitInfo.SignalSemaphores.size());
        for (const auto& [semaphore, stage, value] : submitInfo.SignalSemaphores) {
            auto semaphoreInfo = VkSemaphoreSubmitInfo(VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO);
            semaphoreInfo.semaphore = semaphore.get().GetHandle();
            semaphoreInfo.stageMask = ToEnumCounterpart(stage);
            semaphoreInfo.value = value;
            signalSemaphoreInfos.emplace_back(semaphoreInfo);
        }

        if (submitInfo.Timeline) {
            const auto& timeline = submitInfo.Timeline->get();
            auto semaphoreInfo = VkSemaphoreSubmitInfo(VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO);
            semaphoreInfo.semaphore = timeline.GetDeviceTimelineSemaphore().GetHandle();
            semaphoreInfo.stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            semaphoreInfo.value = timeline.GetNextSignalTimelineValue();
            signalSemaphoreInfos.emplace_back(semaphoreInfo);
        }

        auto commandBufferInfos = std::vector<VkCommandBufferSubmitInfo>();
        commandBufferInfos.reserve(submitInfo.CommandBuffers.size());
        for (const auto& commandBuffer : submitInfo.CommandBuffers) {
            auto commandBufferInfo = VkCommandBufferSubmitInfo(VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO);
            commandBufferInfo.commandBuffer = commandBuffer.get().GetHandle();
            commandBufferInfos.emplace_back(commandBufferInfo);
        }

        const auto fenceHandle = fence
            ? fence->GetHandle()
            : VkFence();

        auto queueSubmitInfo = VkSubmitInfo2(VK_STRUCTURE_TYPE_SUBMIT_INFO_2);
        queueSubmitInfo.waitSemaphoreInfoCount = waitSemaphoreInfos.size();
        queueSubmitInfo.pWaitSemaphoreInfos = waitSemaphoreInfos.data();
        queueSubmitInfo.commandBufferInfoCount = commandBufferInfos.size();
        queueSubmitInfo.pCommandBufferInfos = commandBufferInfos.data();
        queueSubmitInfo.signalSemaphoreInfoCount = signalSemaphoreInfos.size();
        queueSubmitInfo.pSignalSemaphoreInfos = signalSemaphoreInfos.data();
        auto guard = std::lock_guard(_lock);
        RETINA_VULKAN_CHECK(*_logger, vkQueueSubmit2(_handle, 1, &queueSubmitInfo, fenceHandle));
    }

    auto CQueue::Submit(std::function<void(CCommandBuffer&)>&& callback) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        auto commandBuffer = CCommandBuffer::Make(*this, {
            .Name = "TransientCommandBuffer",
            .CommandPoolInfo = { {
                .Flags = ECommandPoolCreateFlag::E_TRANSIENT,
            } }
        });
        commandBuffer->Begin();
        callback(*commandBuffer);
        commandBuffer->End();
        auto fence = CFence::Make(GetDevice(), {
            .Name = "TransientSubmissionFence",
        });
        Submit({
            .CommandBuffers = { *commandBuffer },
        }, fence.Get());
        fence->Wait();
    }

    auto CQueue::Present(const SQueuePresentInfo& presentInfo) noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        auto waitSemaphoreHandles = std::vector<VkSemaphore>();
        waitSemaphoreHandles.reserve(presentInfo.WaitSemaphores.size());
        for (const auto& semaphore : presentInfo.WaitSemaphores) {
            waitSemaphoreHandles.emplace_back(semaphore.get().GetHandle());
        }

        const auto& swapchain = presentInfo.Swapchain.get();
        const auto swapchainHandle = swapchain.GetHandle();
        const auto imageIndex = swapchain.GetCurrentImageIndex();
        auto queuePresentInfo = VkPresentInfoKHR(VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
        queuePresentInfo.waitSemaphoreCount = waitSemaphoreHandles.size();
        queuePresentInfo.pWaitSemaphores = waitSemaphoreHandles.data();
        queuePresentInfo.swapchainCount = 1;
        queuePresentInfo.pSwapchains = &swapchainHandle;
        queuePresentInfo.pImageIndices = &imageIndex;
        _lock.lock();
        const auto result = vkQueuePresentKHR(_handle, &queuePresentInfo);
        _lock.unlock();
        if (result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_ERROR_SURFACE_LOST_KHR ||
            result == VK_SUBOPTIMAL_KHR
        ) {
            return false;
        }
        if (result == VK_SUCCESS) {
            return true;
        }
        RETINA_VULKAN_CHECK(*_logger, result);
        std::unreachable();
    }
}
