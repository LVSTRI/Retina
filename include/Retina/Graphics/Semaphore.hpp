#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/SemaphoreInfo.hpp>

#include <vulkan/vulkan.h>

namespace Retina {
    class ISemaphore : public INativeDebugName, public IEnableIntrusiveReferenceCount<ISemaphore> {
    public:
        using Self = ISemaphore;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkSemaphore;
        RETINA_NODISCARD auto GetKind() const noexcept -> ESemaphoreKind;
        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

        auto SetDebugName(std::string_view name) noexcept -> void;

    protected:
        ISemaphore(ESemaphoreKind kind) noexcept;
        virtual ~ISemaphore() noexcept;

        VkSemaphore _handle = {};
        ESemaphoreKind _kind = {};

        CArcPtr<const CDevice> _device;
    };
}
