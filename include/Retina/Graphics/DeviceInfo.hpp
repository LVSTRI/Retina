#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Enum.hpp>

#include <string>

namespace Retina::Graphics {
  struct SDeviceRayTracingProperties {
    uint32 ShaderGroupHandleSize = 0;
    uint32 MaxRayRecursionDepth = 0;
    uint32 MaxShaderGroupStride = 0;
    uint32 ShaderGroupBaseAlignment = 0;
    uint32 ShaderGroupHandleCaptureReplaySize = 0;
    uint32 MaxRayDispatchInvocationCount = 0;
    uint32 ShaderGroupHandleAlignment = 0;
    uint32 MaxRayHitAttributeSize = 0;
  };

  struct SDeviceMemoryType {
    EMemoryPropertyFlag Flags = {};
    uint64 Heap = 0;
  };

  struct SDeviceMemoryHeap {
    EMemoryHeapFlag Flags = {};
    uint64 Size = 0;
  };

  struct SDeviceHeapProperties {
    std::vector<uint64> Sizes = {};
  };

  struct SDeviceHeapBudget {
    uint64 Budget = 0;
    uint64 Usage = 0;
  };

  struct SDeviceFeature {
    bool Swapchain = false;
    bool MeshShader = false;
    bool RayTracingPipeline = false;
    bool AccelerationStructure = false;
    bool MemoryBudget = false;
    bool MemoryPriority = false;
  };

  struct SDeviceCreateInfo {
    std::string Name;
    SDeviceFeature Features = {};
  };
}
