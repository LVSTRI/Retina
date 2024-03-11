add_library(Retina.Core STATIC)

target_sources(Retina.Core PRIVATE
  Logger.cpp
)

target_link_libraries(Retina.Core PRIVATE
  Retina.Configuration
  Retina.Dependencies
)
