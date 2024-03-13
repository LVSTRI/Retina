set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
include(FetchContent)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

# debugbreak
FetchContent_Declare(
  debugbreak
  GIT_REPOSITORY https://github.com/scottt/debugbreak
  GIT_TAG 5dcbe41d2bd4712c8014aa7e843723ad7b40fd74
  GIT_SHALLOW TRUE
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
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(mio)

FetchContent_Declare(
  fmtlib
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG df6e7b22f7fef9c37736b3718c5ccb357df7dcd2
  GIT_SHALLOW TRUE
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
  GIT_TAG 7e635fca68d014934b4af8a1cf874f63989352b7
  GIT_SHALLOW TRUE
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
  GIT_SHALLOW TRUE
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
  GIT_SHALLOW TRUE
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
  GIT_SHALLOW TRUE
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
  GIT_SHALLOW TRUE
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
  GIT_SHALLOW TRUE
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
  GIT_TAG d911053e390816ecc5dedd5a9d6b4bb5ed92b4c9
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(unordered_dense)

add_library(Retina.Dependencies INTERFACE)

target_link_libraries(Retina.Dependencies INTERFACE
  debugbreak
  glm
  mio
  spdlog
  TracyClient
  unordered_dense
)

add_library(Retina.WSI.Dependencies INTERFACE)

target_link_libraries(Retina.WSI.Dependencies INTERFACE
  glfw
)

add_library(Retina.Graphics.Dependencies INTERFACE)

target_link_libraries(Retina.Graphics.Dependencies INTERFACE
  spirv-cross-glsl
  volk
  Vulkan::shaderc_combined
)

add_library(Retina.Graphics.Vulkan INTERFACE)

target_include_directories(Retina.Graphics.Vulkan INTERFACE ${Vulkan_INCLUDE_DIRS})
target_compile_definitions(Retina.Graphics.Vulkan INTERFACE VK_NO_PROTOTYPES)
target_link_libraries(Retina.Graphics.Vulkan INTERFACE vma)