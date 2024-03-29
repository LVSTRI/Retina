#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Enum.hpp>
#include <Retina/Graphics/Forward.hpp>

#include <string>

namespace Retina::Graphics {
  enum class EQueueDomain {
    E_GRAPHICS,
    E_COMPUTE,
    E_TRANSFER,
  };

  struct SQueueCreateInfo {
    std::string Name;
    EQueueDomain Domain = EQueueDomain::E_GRAPHICS;
    uint32 FamilyIndex = 0;
    uint32 QueueIndex = 0;
  };

  struct SQueueSemaphoreSubmitInfo {
    Core::CReferenceWrapper<const ISemaphore> Semaphore;
    EPipelineStageFlag Stage = EPipelineStageFlag::E_NONE;
    uint64 Value = -1_u64;
  };

  struct SQueueSubmitInfo {
    std::vector<Core::CReferenceWrapper<const CCommandBuffer>> CommandBuffers;
    std::vector<SQueueSemaphoreSubmitInfo> WaitSemaphores;
    std::vector<SQueueSemaphoreSubmitInfo> SignalSemaphores;
    std::vector<Core::CReferenceWrapper<CHostDeviceTimeline>> Timelines;
  };
}
