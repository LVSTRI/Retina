#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Semaphore.hpp>

#include <volk.h>

namespace Retina::Graphics {
  ISemaphore::ISemaphore(ESemaphoreKind kind) noexcept
    : _kind(kind)
  {
    RETINA_PROFILE_SCOPED();
  }

  ISemaphore::~ISemaphore() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      vkDestroySemaphore(_device->GetHandle(), _handle, nullptr);
    }
  }

  auto ISemaphore::GetHandle() const noexcept -> VkSemaphore {
    RETINA_PROFILE_SCOPED();
    return _handle;
  }

  auto ISemaphore::GetKind() const noexcept -> ESemaphoreKind {
    RETINA_PROFILE_SCOPED();
    return _kind;
  }

  auto ISemaphore::GetDevice() const noexcept -> const CDevice& {
    RETINA_PROFILE_SCOPED();
    return *_device;
  }
}
