#include <Retina/Graphics/BinarySemaphore.hpp>
#include <Retina/Graphics/CommandBuffer.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Fence.hpp>
#include <Retina/Graphics/HostDeviceTimeline.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>
#include <Retina/Graphics/Queue.hpp>
#include <Retina/Graphics/TimelineSemaphore.hpp>

#include <volk.h>

namespace Retina::Graphics {
  CQueue::CQueue(const CDevice& device) noexcept
    : _device(device)
  {
    RETINA_PROFILE_SCOPED();
  }

  CQueue::~CQueue() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      RETINA_GRAPHICS_INFO("Queue ({}) destroyed", GetDebugName());
    }
  }

  auto CQueue::Make(const CDevice& device, const SQueueCreateInfo& createInfo) noexcept -> Core::CArcPtr<CQueue> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CQueue(device));

    auto queueHandle = VkQueue();
    vkGetDeviceQueue(device.GetHandle(), createInfo.FamilyIndex, createInfo.QueueIndex, &queueHandle);
    RETINA_GRAPHICS_INFO(
      "Queue ({}) initialized with: {{ Family: {}, Index: {} }}",
      createInfo.Name,
      createInfo.FamilyIndex,
      createInfo.QueueIndex
    );

    self->_handle = queueHandle;
    self->_createInfo = createInfo;
    self->SetDebugName(createInfo.Name);
    return self;
  }

  auto CQueue::GetHandle() const noexcept -> VkQueue {
    RETINA_PROFILE_SCOPED();
    return _handle;
  }

  auto CQueue::GetCreateInfo() const noexcept -> const SQueueCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CQueue::GetDevice() const noexcept -> const CDevice& {
    RETINA_PROFILE_SCOPED();
    return _device;
  }

  auto CQueue::GetDomain() const noexcept -> EQueueDomain {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Domain;
  }

  auto CQueue::GetFamilyIndex() const noexcept -> uint32 {
    RETINA_PROFILE_SCOPED();
    return _createInfo.FamilyIndex;
  }

  auto CQueue::GetIndex() const noexcept -> uint32 {
    RETINA_PROFILE_SCOPED();
    return _createInfo.QueueIndex;
  }

  auto CQueue::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CQueue::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_DEBUG_NAME(GetDevice().GetHandle(), _handle, VK_OBJECT_TYPE_QUEUE, name);
    _createInfo.Name = name;
  }

  auto CQueue::Submit(const SQueueSubmitInfo& submitInfo, const CFence* fence) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    auto waitSemaphoreInfos = std::vector<VkSemaphoreSubmitInfo>();
    waitSemaphoreInfos.reserve(submitInfo.WaitSemaphores.size());
    for (const auto& [semaphore, stage, value] : submitInfo.WaitSemaphores) {
      auto semaphoreSubmitInfo = VkSemaphoreSubmitInfo(VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO);
      semaphoreSubmitInfo.semaphore = semaphore->GetHandle();
      semaphoreSubmitInfo.stageMask = AsEnumCounterpart(stage);
      semaphoreSubmitInfo.value = value;
      waitSemaphoreInfos.emplace_back(semaphoreSubmitInfo);
    }

    auto signalSemaphoreInfos = std::vector<VkSemaphoreSubmitInfo>();
    signalSemaphoreInfos.reserve(submitInfo.SignalSemaphores.size());
    for (const auto& [semaphore, stage, value] : submitInfo.SignalSemaphores) {
      auto semaphoreSubmitInfo = VkSemaphoreSubmitInfo(VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO);
      semaphoreSubmitInfo.semaphore = semaphore->GetHandle();
      semaphoreSubmitInfo.stageMask = AsEnumCounterpart(stage);
      semaphoreSubmitInfo.value = value;
      signalSemaphoreInfos.emplace_back(semaphoreSubmitInfo);
    }

    for (auto& timeline : submitInfo.Timelines) {
      auto semaphoreInfo = VkSemaphoreSubmitInfo(VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO);
      semaphoreInfo.semaphore = timeline->GetDeviceTimeline().GetHandle();
      semaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_NONE;
      semaphoreInfo.value = timeline->GetNextHostTimelineValue();
      signalSemaphoreInfos.emplace_back(semaphoreInfo);
    }

    auto commandBufferInfos = std::vector<VkCommandBufferSubmitInfo>();
    commandBufferInfos.reserve(submitInfo.CommandBuffers.size());
    for (const auto& commandBuffer : submitInfo.CommandBuffers) {
      auto commandBufferInfo = VkCommandBufferSubmitInfo(VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO);
      commandBufferInfo.commandBuffer = commandBuffer->GetHandle();
      commandBufferInfos.emplace_back(commandBufferInfo);
    }

    const auto fenceHandle = fence ? fence->GetHandle() : VkFence();

    auto queueSubmitInfo = VkSubmitInfo2(VK_STRUCTURE_TYPE_SUBMIT_INFO_2);
    queueSubmitInfo.waitSemaphoreInfoCount = waitSemaphoreInfos.size();
    queueSubmitInfo.pWaitSemaphoreInfos = waitSemaphoreInfos.data();
    queueSubmitInfo.commandBufferInfoCount = commandBufferInfos.size();
    queueSubmitInfo.pCommandBufferInfos = commandBufferInfos.data();
    queueSubmitInfo.signalSemaphoreInfoCount = signalSemaphoreInfos.size();
    queueSubmitInfo.pSignalSemaphoreInfos = signalSemaphoreInfos.data();
    auto guard = std::lock_guard(_mutex);
    RETINA_GRAPHICS_VULKAN_CHECK(vkQueueSubmit2(_handle, 1, &queueSubmitInfo, fenceHandle));
  }

  auto CQueue::WaitIdle() const noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_VULKAN_CHECK(vkQueueWaitIdle(_handle));
  }

  auto CQueue::Lock() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _mutex.lock();
  }

  auto CQueue::Unlock() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    _mutex.unlock();
  }
}

