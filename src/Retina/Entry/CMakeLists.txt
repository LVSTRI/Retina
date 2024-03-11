add_executable(Retina.Entry)

target_sources(Retina.Entry PRIVATE
  Application.cpp
  main.cpp
)

target_link_libraries(Retina.Entry PRIVATE
  Retina.Configuration
  Retina.Core
  Retina.Dependencies
  Retina.Graphics
  Retina.WSI
)
