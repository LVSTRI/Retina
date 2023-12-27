#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/Semaphore.hpp>

#include <vulkan/vulkan.h>

namespace Retina {
    class CTimelineSemaphore : public ISemaphore {
    public:
        using Self = CTimelineSemaphore;

        CTimelineSemaphore() noexcept;
        ~CTimelineSemaphore() noexcept override = default;

        RETINA_NODISCARD static auto Make(const CDevice& device, const STimelineSemaphoreCreateInfo& createInfo) noexcept -> CArcPtr<Self>;
        RETINA_NODISCARD static auto Make(
            const CDevice& device,
            uint32 count,
            const STimelineSemaphoreCreateInfo& createInfo
        ) noexcept -> std::vector<CArcPtr<Self>>;

        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const STimelineSemaphoreCreateInfo&;

        RETINA_NODISCARD auto GetCounter() const noexcept -> uint64;
        auto Wait(uint64 value, uint64 timeout = -1_u64) const noexcept -> bool;
        auto Signal(uint64 value) const noexcept -> void;

    private:
        STimelineSemaphoreCreateInfo _createInfo = {};
    };
}
