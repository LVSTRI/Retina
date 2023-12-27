#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/Semaphore.hpp>

#include <vulkan/vulkan.h>

namespace Retina {
    class CBinarySemaphore : public ISemaphore {
    public:
        using Self = CBinarySemaphore;

        CBinarySemaphore() noexcept;
        ~CBinarySemaphore() noexcept override = default;

        RETINA_NODISCARD static auto Make(const CDevice& device, const SBinarySemaphoreCreateInfo& createInfo) noexcept -> CArcPtr<Self>;
        RETINA_NODISCARD static auto Make(
            const CDevice& device,
            uint32 count,
            const SBinarySemaphoreCreateInfo& createInfo
        ) noexcept -> std::vector<CArcPtr<Self>>;

        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SBinarySemaphoreCreateInfo&;

    private:
        SBinarySemaphoreCreateInfo _createInfo = {};
    };
}
