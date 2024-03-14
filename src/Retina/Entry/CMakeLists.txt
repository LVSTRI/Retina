add_executable(Retina.Entry)

target_sources(Retina.Entry PRIVATE
  Application.cpp
  Camera.cpp
  FrameCounter.cpp
  FrameTimer.cpp
  main.cpp
)

target_link_libraries(Retina.Entry PRIVATE
  Retina.Configuration
  Retina.Core
  Retina.Dependencies
  Retina.Graphics
  Retina.WSI
)

target_compile_definitions(Retina.Entry PRIVATE
  RETINA_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
  RETINA_ASSET_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/Assets"
)
