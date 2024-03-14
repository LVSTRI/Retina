add_library(Retina.Sandbox STATIC)

target_sources(Retina.Sandbox PRIVATE
  Camera.cpp
  FrameCounter.cpp
  FrameTimer.cpp
  Logger.cpp
  MeshletModel.cpp
  Model.cpp
  SandboxApplication.cpp
)

target_link_libraries(Retina.Sandbox PRIVATE
  Retina.Configuration
  Retina.Core
  Retina.Dependencies
  Retina.Entry.Application
  Retina.Graphics
  Retina.Sandbox.Dependencies
  Retina.WSI
)

target_compile_definitions(Retina.Sandbox PRIVATE
  RETINA_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
  RETINA_ASSET_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/Assets"
)

target_link_libraries(Retina.Entry PRIVATE
  Retina.Sandbox
)
