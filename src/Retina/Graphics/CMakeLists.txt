add_library(Retina.Graphics STATIC)

target_sources(Retina.Graphics PRIVATE
  BinarySemaphore.cpp
  CommandBuffer.cpp
  CommandPool.cpp
  DeletionQueue.cpp
  DescriptorPool.cpp
  Device.cpp
  Fence.cpp
  HostDeviceTimeline.cpp
  Image.cpp
  ImageInfo.cpp
  ImageView.cpp
  Instance.cpp
  Logger.cpp
  Queue.cpp
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
