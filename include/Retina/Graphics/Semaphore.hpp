#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Forward.hpp>
#include <Retina/Graphics/SemaphoreInfo.hpp>

#include <vulkan/vulkan.h>

namespace Retina::Graphics {
  class ISemaphore : public Core::IEnableIntrusiveReferenceCount<ISemaphore> {
  public:
    virtual ~ISemaphore() noexcept;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkSemaphore;
    RETINA_NODISCARD auto GetKind() const noexcept -> ESemaphoreKind;
    RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

  protected:
    ISemaphore(const CDevice& device, ESemaphoreKind kind) noexcept;

    VkSemaphore _handle = {};
    ESemaphoreKind _kind = {};

    Core::CReferenceWrapper<const CDevice> _device;
  };
}
