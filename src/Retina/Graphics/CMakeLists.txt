add_library(Retina.Graphics STATIC)

target_sources(Retina.Graphics PRIVATE
  Resources/ShaderResourceTable.cpp
  BinarySemaphore.cpp
  Buffer.cpp
  CommandBuffer.cpp
  CommandPool.cpp
  ComputePipeline.cpp
  DeletionQueue.cpp
  DescriptorLayout.cpp
  DescriptorPool.cpp
  DescriptorSet.cpp
  Device.cpp
  Fence.cpp
  GraphicsPipeline.cpp
  HostDeviceTimeline.cpp
  Image.cpp
  ImageInfo.cpp
  ImageView.cpp
  Instance.cpp
  Logger.cpp
  MeshShadingPipeline.cpp
  Pipeline.cpp
  Queue.cpp
  Sampler.cpp
  Semaphore.cpp
  Swapchain.cpp
  TimelineSemaphore.cpp
)

target_link_libraries(Retina.Graphics PRIVATE
  Retina.Configuration
  Retina.Core
  Retina.Dependencies
  Retina.Graphics.Dependencies
)

target_link_libraries(Retina.Graphics PUBLIC
  Retina.Graphics.Vulkan
)
