add_library(Retina.GUI STATIC)

target_sources(Retina.GUI PRIVATE
  ImGuiContext.cpp
  Logger.cpp
)

target_link_libraries(Retina.GUI PRIVATE
  Retina.Configuration
  Retina.Core
  Retina.Dependencies
  Retina.Graphics
  Retina.WSI
)

target_link_libraries(Retina.GUI PUBLIC
  Retina.GUI.Dependencies
)

target_compile_definitions(Retina.GUI PRIVATE
  RETINA_GUI_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
  RETINA_GUI_FONT_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/Fonts"
)
