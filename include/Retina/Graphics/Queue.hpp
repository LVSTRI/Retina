#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/QueueInfo.hpp>

#include <vulkan/vulkan.h>

#include <mutex>

namespace Retina::Graphics {
  class CQueue : public Core::IEnableIntrusiveReferenceCount<CQueue> {
  public:
    CQueue(const CDevice& device) noexcept;
    ~CQueue() noexcept;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      const SQueueCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CQueue>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkQueue;
    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SQueueCreateInfo&;
    RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

    RETINA_NODISCARD auto GetDomain() const noexcept -> EQueueDomain;
    RETINA_NODISCARD auto GetFamilyIndex() const noexcept -> uint32;
    RETINA_NODISCARD auto GetIndex() const noexcept -> uint32;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

    auto Submit(const SQueueSubmitInfo& submitInfo, const CFence* fence = nullptr) noexcept -> void;
    auto Submit(std::move_only_function<void(CCommandBuffer&)>&& submission) noexcept -> void;

    auto WaitIdle() const noexcept -> void;
    auto Lock() noexcept -> void;
    auto Unlock() noexcept -> void;

  private:
    VkQueue _handle = {};
    std::mutex _mutex;

    SQueueCreateInfo _createInfo = {};
    Core::CReferenceWrapper<const CDevice> _device;
  };
}
