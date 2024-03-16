add_library(Retina.Configuration INTERFACE)

include(CheckIPOSupported)
check_ipo_supported(RESULT IS_IPO_SUPPORTED)

if (IS_IPO_SUPPORTED)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(Retina.Configuration INTERFACE -flto=full)
    target_link_options(Retina.Configuration INTERFACE -flto=full -flto-jobs=0)
    if (NOT WIN32)
      target_compile_options(Retina.Configuration INTERFACE -ffat-lto-objects)
      target_link_options(Retina.Configuration INTERFACE -ffat-lto-objects)
    endif ()
  else ()
    target_compile_options(Retina.Configuration INTERFACE -flto=auto -ffat-lto-objects -flto-partition=none)
    target_link_options(Retina.Configuration INTERFACE -flto=auto -ffat-lto-objects -flto-partition=none)
  endif ()
endif ()

# Set include directories
target_include_directories(Retina.Configuration INTERFACE ${RETINA_INCLUDE_DIRECTORY})

# Set compiler options
set(RETINA_CONFIGURATION_COMPILE_OPTIONS)

set(RETINA_CONFIGURATION_COMPILE_OPTIONS ${RETINA_CONFIGURATION_COMPILE_OPTIONS}
  -Wall
  -Wextra
  -pedantic

  -Wno-missing-field-initializers

  -march=native
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(RETINA_CONFIGURATION_COMPILE_OPTIONS ${RETINA_CONFIGURATION_COMPILE_OPTIONS}
    -Wno-language-extension-token
    -Wno-gnu-statement-expression
    -Wno-nullability-extension
  )
endif ()
if (WIN32)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(RETINA_CONFIGURATION_COMPILE_OPTIONS ${RETINA_CONFIGURATION_COMPILE_OPTIONS}
      -fms-runtime-lib=dll_dbg
    )
  else ()
    set(RETINA_CONFIGURATION_COMPILE_OPTIONS ${RETINA_CONFIGURATION_COMPILE_OPTIONS}
      -fms-runtime-lib=dll
    )
  endif ()
endif ()
target_compile_options(Retina.Configuration INTERFACE ${RETINA_CONFIGURATION_COMPILE_OPTIONS})

# Set precompiled headers
target_link_libraries(Retina.Configuration INTERFACE Retina.PrecompiledHeaders)

set(RETINA_CONFIGURATION_COMPILE_DEFINITIONS)
if (${RETINA_ENABLE_PROFILER})
  set(RETINA_CONFIGURATION_COMPILE_DEFINITIONS ${RETINA_CONFIGURATION_COMPILE_DEFINITIONS} "RETINA_ENABLE_PROFILER")
endif ()
if (${RETINA_ENABLE_LOGGER})
  set(RETINA_CONFIGURATION_COMPILE_DEFINITIONS ${RETINA_CONFIGURATION_COMPILE_DEFINITIONS} "RETINA_ENABLE_LOGGER")
endif ()
if (WIN32)
  set(RETINA_CONFIGURATION_COMPILE_DEFINITIONS ${RETINA_CONFIGURATION_COMPILE_DEFINITIONS} "_CRT_SECURE_NO_WARNINGS")
endif ()
set(RETINA_CONFIGURATION_COMPILE_DEFINITIONS ${RETINA_CONFIGURATION_COMPILE_DEFINITIONS}
  RETINA_MAIN_SHADER_DIRECTORY="${RETINA_MAIN_SHADER_DIRECTORY}"
)

target_compile_definitions(Retina.Configuration INTERFACE ${RETINA_CONFIGURATION_COMPILE_DEFINITIONS})

add_subdirectory(Core)
add_subdirectory(Entry)
add_subdirectory(Graphics)
add_subdirectory(GUI)
add_subdirectory(Sandbox)
add_subdirectory(WSI)
