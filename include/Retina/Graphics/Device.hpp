#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/DeletionQueue.hpp>
#include <Retina/Graphics/DeviceInfo.hpp>
#include <Retina/Graphics/Forward.hpp>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Retina::Graphics {
  class CDevice : public Core::IEnableIntrusiveReferenceCount<CDevice> {
  public:
    CDevice() noexcept = default;
    ~CDevice() noexcept;

    RETINA_NODISCARD static auto Make(
      const CInstance& instance,
      const SDeviceCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CDevice>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkDevice;
    RETINA_NODISCARD auto GetPhysicalDevice() const noexcept -> VkPhysicalDevice;
    RETINA_NODISCARD auto GetAllocator() const noexcept -> VmaAllocator;

    RETINA_NODISCARD auto GetGraphicsQueue() const noexcept -> CQueue&;
    RETINA_NODISCARD auto GetComputeQueue() const noexcept -> CQueue&;
    RETINA_NODISCARD auto GetTransferQueue() const noexcept -> CQueue&;

    RETINA_NODISCARD auto GetMainTimeline() const noexcept -> CHostDeviceTimeline&;
    RETINA_NODISCARD auto GetDeletionQueue() const noexcept -> CDeletionQueue&;

    template <typename T>
    RETINA_NODISCARD auto GetRayTracingProperty(T SDeviceRayTracingProperties::* property) const noexcept -> T;

    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SDeviceCreateInfo&;
    RETINA_NODISCARD auto GetInstance() const noexcept -> const CInstance&;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

    RETINA_NODISCARD auto IsFeatureEnabled(bool SDeviceFeature::* feature) const noexcept -> bool;

    auto WaitIdle() const noexcept -> void;

    auto Tick() noexcept -> void;

  private:
    VkDevice _handle = {};
    VkPhysicalDevice _physicalDevice = {};
    VmaAllocator _allocator = {};

    Core::CArcPtr<CQueue> _graphicsQueue;
    Core::CArcPtr<CQueue> _computeQueue;
    Core::CArcPtr<CQueue> _transferQueue;

    std::unique_ptr<CHostDeviceTimeline> _mainTimeline;
    std::unique_ptr<CDeletionQueue> _deletionQueue;

    SDeviceRayTracingProperties _rayTracingProperties = {};
    SDeviceCreateInfo _createInfo = {};
    Core::CArcPtr<const CInstance> _instance;
  };

  template <typename T>
  auto CDevice::GetRayTracingProperty(T SDeviceRayTracingProperties::* property) const noexcept -> T {
    return _rayTracingProperties.*property;
  }
}
