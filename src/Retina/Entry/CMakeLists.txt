add_library(Retina.Entry.Application STATIC)

target_sources(Retina.Entry.Application PRIVATE
  Application.cpp
)

target_link_libraries(Retina.Entry.Application PRIVATE
  Retina.Configuration
  Retina.Dependencies
)

add_executable(Retina.Entry)

target_sources(Retina.Entry PRIVATE
  main.cpp
)

target_link_libraries(Retina.Entry PRIVATE
  Retina.Configuration
  Retina.Dependencies
  Retina.Entry.Application
)
