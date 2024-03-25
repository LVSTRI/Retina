set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
include(FetchContent)

find_package(Vulkan REQUIRED COMPONENTS
  SPIRV-Tools
  shaderc_combined
)

# debugbreak
FetchContent_Declare(
  debugbreak
  GIT_REPOSITORY https://github.com/scottt/debugbreak
  GIT_TAG 5dcbe41d2bd4712c8014aa7e843723ad7b40fd74
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
FetchContent_GetProperties(debugbreak)
if (NOT debugbreak_POPULATED)
  FetchContent_Populate(debugbreak)
  add_library(debugbreak INTERFACE)
  target_include_directories(debugbreak INTERFACE ${debugbreak_SOURCE_DIR})
endif ()

# mio
FetchContent_Declare(
  mio
  GIT_REPOSITORY https://github.com/vimpunk/mio
  GIT_TAG 8b6b7d878c89e81614d05edca7936de41ccdd2da
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(mio)

FetchContent_Declare(
  fmtlib
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG e69e5f977d458f2650bb346dadf2ad30c5320281
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(fmtlib)
target_compile_options(fmt PRIVATE -w)

# spdlog
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog
  GIT_TAG 7c02e204c92545f869e2f04edaab1f19fe8b19fd
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)
target_compile_options(spdlog PRIVATE -w)
target_link_libraries(spdlog PUBLIC fmt)

# glm
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm
  GIT_TAG 0af55ccecd98d4e5a8d1fad7de25ba429d60e863
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glm)
target_compile_definitions(glm INTERFACE
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_RADIANS
  GLM_FORCE_AVX2
)
target_compile_options(glm INTERFACE -w)

# tracy
FetchContent_Declare(
  tracy
  GIT_REPOSITORY https://github.com/wolfpld/tracy
  GIT_TAG 37aff70dfa50cf6307b3fee6074d627dc2929143
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
set(TRACY_CALLSTACK ON CACHE BOOL "" FORCE)
set(TRACY_ONLY_LOCALHOST ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(tracy)
target_compile_options(TracyClient PRIVATE -w)

# glfw
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw
  GIT_TAG 7b6aead9fb88b3623e3b3725ebb42670cbe4c579
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# volk
FetchContent_Declare(
  volk
  GIT_REPOSITORY https://github.com/zeux/volk
  GIT_TAG dd19b95a0ce6f66ca3172765f9e20d3f9ec9272a
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(volk)
target_include_directories(volk PUBLIC ${Vulkan_INCLUDE_DIRS})

# vma
FetchContent_Declare(
  vma
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
  GIT_TAG 19b940e864bd3a5afb3c79e3c6788869d01a19eb
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
FetchContent_GetProperties(vma)
if (NOT vma_POPULATED)
  set(VMA_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
  set(VMA_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Populate(vma)
  add_subdirectory(${vma_SOURCE_DIR} ${vma_BINARY_DIR})

  add_library(vma STATIC)
  target_sources(vma PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/VMA/Implementation.cpp
  )
  target_include_directories(vma PUBLIC
    ${Vulkan_INCLUDE_DIRS}
    ${vma_SOURCE_DIR}/include
  )
  target_link_libraries(vma PRIVATE volk)
  target_compile_options(vma PRIVATE -w)
endif ()

# SPIRV-Cross
FetchContent_Declare(
    spirv_cross
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross
    GIT_TAG        2a7c8184921897ff3d6c6c3f70af4099e2e00331
    GIT_PROGRESS   TRUE
    GIT_SHALLOW    TRUE
)
set(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spirv_cross)

# unordered_dense
FetchContent_Declare(
  unordered_dense
  GIT_REPOSITORY https://github.com/martinus/unordered_dense
  GIT_TAG 231e48c9426bd21c273669e5fdcd042c146975cf
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(unordered_dense)

# cgltf
FetchContent_Declare(
  CGLTF
  GIT_REPOSITORY https://github.com/jkuhlmann/cgltf
  GIT_TAG de399881c65c438a635627c749440eeea7e05599
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
FetchContent_GetProperties(cgltf)
if (NOT cgltf_POPULATED)
  FetchContent_Populate(cgltf)
  add_library(cgltf STATIC)
  target_sources(cgltf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/CGLTF/Implementation.c
  )
  target_include_directories(cgltf PUBLIC
    ${cgltf_SOURCE_DIR}
  )
  target_compile_options(cgltf PRIVATE -w)
endif ()

# basisu
FetchContent_Declare(
  basisu
  GIT_REPOSITORY https://github.com/BinomialLLC/basis_universal
  GIT_TAG 900e40fb5d2502927360fe2f31762bdbb624455f
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)

# meshoptimizer
FetchContent_Declare(
  meshoptimizer
  GIT_REPOSITORY https://github.com/LVSTRI/meshoptimizer
  GIT_TAG 084c774e2db01937a9121cb132b7091f7cb26221
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
set(MESHOPT_BASISU_PATH ${basisu_SOURCE_DIR} CACHE PATH "" FORCE)
set(MESHOPT_BUILD_GLTFPACK ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(meshoptimizer)
target_compile_options(meshoptimizer PRIVATE -w)
target_compile_options(gltfpack PRIVATE -w)

# imgui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui
  GIT_TAG adcc3217c38142e965eae679a299a34ce1c963d5
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
FetchContent_GetProperties(imgui)
if (NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
  add_library(imgui STATIC)
  target_sources(imgui PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  )
  target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
  )
  target_compile_options(imgui PRIVATE -w)
  target_link_libraries(imgui PRIVATE glfw)
endif ()

# NVIDIA DLSS
FetchContent_Declare(
  nvdia_dlss
  GIT_REPOSITORY https://github.com/NVIDIA/DLSS
  GIT_TAG 2aa0b2c9d07995f517561ab6bbef60308bc7eb3a
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
FetchContent_GetProperties(nvdia_dlss)
if (NOT nvdia_dlss_POPULATED)
  FetchContent_Populate(nvdia_dlss)
  add_library(nvdia_dlss INTERFACE)

  set(RETINA_DLSS_LIBRARY_DIRECTORY ${nvdia_dlss_SOURCE_DIR}/lib)
  if (WIN32)
    set(RETINA_DLSS_LIBRARY_DIRECTORY ${RETINA_DLSS_LIBRARY_DIRECTORY}/Windows_x86_64)
    set(RETINA_DLSS_LIBRARY_STATIC_DIRECTORY ${RETINA_DLSS_LIBRARY_DIRECTORY}/x86_64)
    set(RETINA_DLSS_LIBRARY_DYNAMIC_NAME nvngx_dlss.dll)
  else ()
    set(RETINA_DLSS_LIBRARY_DIRECTORY ${RETINA_DLSS_LIBRARY_DIRECTORY}/Linux_x86_64)
    set(RETINA_DLSS_LIBRARY_STATIC_DIRECTORY ${RETINA_DLSS_LIBRARY_DIRECTORY})
    set(RETINA_DLSS_LIBRARY_DYNAMIC_NAME libnvidia-ngx-dlss.so.3.5.10)
  endif ()

  if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(RETINA_DLSS_LIBRARY_DYNAMIC_DIRECTORY ${RETINA_DLSS_LIBRARY_DIRECTORY}/dev)
  else ()
    set(RETINA_DLSS_LIBRARY_DYNAMIC_DIRECTORY ${RETINA_DLSS_LIBRARY_DIRECTORY}/rel)
  endif ()

  if (WIN32)
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
      set(RETINA_DLSS_LIBRARY_STATIC_NAME nvsdk_ngx_d_dbg)
    else ()
      set(RETINA_DLSS_LIBRARY_STATIC_NAME nvsdk_ngx_d)
    endif ()
  else ()
    set(RETINA_DLSS_LIBRARY_STATIC_NAME libnvsdk_ngx)
  endif ()

  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy
      ${RETINA_DLSS_LIBRARY_DYNAMIC_DIRECTORY}/${RETINA_DLSS_LIBRARY_DYNAMIC_NAME}
      ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${RETINA_DLSS_LIBRARY_DYNAMIC_NAME}
  )

  target_link_directories(nvdia_dlss INTERFACE ${RETINA_DLSS_LIBRARY_STATIC_DIRECTORY})
  target_link_libraries(nvdia_dlss INTERFACE ${RETINA_DLSS_LIBRARY_STATIC_NAME})
  target_include_directories(nvdia_dlss INTERFACE ${nvdia_dlss_SOURCE_DIR}/include)
endif ()

# KTX changes global variables CMAKE_ARCHIVE_OUTPUT_DIRECTORY, CMAKE_LIBRARY_OUTPUT_DIRECTORY and
# CMAKE_RUNTIME_OUTPUT_DIRECTORY, save them and restore later
set(RETINA_TEMPORARY_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(RETINA_TEMPORARY_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(RETINA_TEMPORARY_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# KTX
FetchContent_Declare(
  ktx
  GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software
  GIT_TAG v4.3.2
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
)
set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_KTX1 ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_KTX2 ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_VK_UPLOAD OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ktx)

# Restore global variables
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${RETINA_TEMPORARY_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${RETINA_TEMPORARY_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RETINA_TEMPORARY_RUNTIME_OUTPUT_DIRECTORY})

if (WIN32)
  set(KTX_LIBRARY_DYNAMIC_NAME ktx_read.dll)
else ()
  set(KTX_LIBRARY_DYNAMIC_NAME libktx_read.so)
endif ()

add_custom_target(ktx_read_copy
  COMMAND ${CMAKE_COMMAND} -E copy
    "${RETINA_ROOT_BINARY_DIRECTORY}/${CMAKE_BUILD_TYPE}/${KTX_LIBRARY_DYNAMIC_NAME}"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${KTX_LIBRARY_DYNAMIC_NAME}"
  DEPENDS ktx_read
)

add_library(Retina.Dependencies INTERFACE)

target_link_libraries(Retina.Dependencies INTERFACE
  debugbreak
  glm
  mio
  spdlog
  unordered_dense
)
if (RETINA_ENABLE_PROFILER)
  target_link_libraries(Retina.Dependencies INTERFACE TracyClient)
endif ()

add_library(Retina.Graphics.Dependencies INTERFACE)

target_link_libraries(Retina.Graphics.Dependencies INTERFACE
  nvdia_dlss
  spirv-cross-glsl
  Vulkan::SPIRV-Tools
  Vulkan::shaderc_combined
)

add_library(Retina.Graphics.Vulkan INTERFACE)

target_include_directories(Retina.Graphics.Vulkan INTERFACE ${Vulkan_INCLUDE_DIRS})
target_compile_definitions(Retina.Graphics.Vulkan INTERFACE VK_NO_PROTOTYPES)
target_link_libraries(Retina.Graphics.Vulkan INTERFACE vma volk)

add_library(Retina.GUI.Dependencies INTERFACE)

target_link_libraries(Retina.GUI.Dependencies INTERFACE
  imgui
)

add_library(Retina.Sandbox.Dependencies INTERFACE)

target_link_libraries(Retina.Sandbox.Dependencies INTERFACE
  cgltf
  meshoptimizer
  ktx_read
)
add_dependencies(Retina.Sandbox.Dependencies ktx_read_copy)

add_library(Retina.WSI.Dependencies INTERFACE)

target_link_libraries(Retina.WSI.Dependencies INTERFACE
  glfw
)
