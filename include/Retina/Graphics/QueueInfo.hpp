#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/SemaphoreInfo.hpp>

namespace Retina {
    enum class EQueueDomain {
        E_GRAPHICS,
        E_COMPUTE,
        E_TRANSFER
    };

    struct SQueueSemaphoreSubmitInfo {
        std::reference_wrapper<const ISemaphore> Semaphore;
        EPipelineStage Stage = EPipelineStage::E_NONE;
        uint64 Value = -1_u64;
    };

    struct SQueueSubmitInfo {
        std::vector<std::reference_wrapper<const CCommandBuffer>> CommandBuffers;
        std::vector<SQueueSemaphoreSubmitInfo> WaitSemaphores;
        std::vector<SQueueSemaphoreSubmitInfo> SignalSemaphores;
        std::optional<std::reference_wrapper<const CSyncHostDeviceTimeline>> Timeline;
    };

    struct SQueuePresentInfo {
        std::reference_wrapper<const CSwapchain> Swapchain;
        std::vector<std::reference_wrapper<const ISemaphore>> WaitSemaphores;
    };

    struct SQueueFamilyInfo {
        using Self = SQueueFamilyInfo;

        uint32 FamilyIndex = 0;
        uint32 QueueIndex = 0;

        RETINA_NODISCARD constexpr auto operator <=>(const Self& other) const noexcept -> std::strong_ordering = default;
    };

    struct SQueueCreateInfo {
        std::string Name;
        EQueueDomain Domain = EQueueDomain::E_GRAPHICS;
        SQueueFamilyInfo FamilyInfo = {};

        RETINA_NODISCARD constexpr auto operator <=>(const SQueueCreateInfo&) const noexcept -> std::strong_ordering = default;
    };
}
