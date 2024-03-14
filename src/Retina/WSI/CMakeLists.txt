add_library(Retina.WSI STATIC)

target_sources(Retina.WSI PRIVATE
  Input.cpp
  Logger.cpp
  Platform.cpp
  Window.cpp
)

target_link_libraries(Retina.WSI PRIVATE
  Retina.Configuration
  Retina.Dependencies
  Retina.WSI.Dependencies
  Retina.Graphics.Vulkan
)
