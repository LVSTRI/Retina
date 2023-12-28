cmake_minimum_required(VERSION 3.26)
project(RetinaDependencies)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE)
include(FetchContent)

FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk
    GIT_TAG        master
    GIT_PROGRESS   TRUE
    GIT_SHALLOW    TRUE
)
set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(volk)
target_include_directories(volk PRIVATE ${Vulkan_INCLUDE_DIRS})

FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy
    GIT_TAG        37aff70dfa50cf6307b3fee6074d627dc2929143
    GIT_PROGRESS   TRUE
    GIT_SHALLOW    TRUE
)
set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
set(TRACY_CALLSTACK ON CACHE BOOL "" FORCE)
set(TRACY_ONLY_LOCALHOST ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(tracy)
target_compile_options(TracyClient PRIVATE -Wno-everything)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG        7e635fca68d014934b4af8a1cf874f63989352b7
    GIT_PROGRESS   TRUE
    GIT_SHALLOW    TRUE
)
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG        master
    GIT_PROGRESS   TRUE
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG        master
    GIT_PROGRESS   TRUE
    GIT_SHALLOW    TRUE
)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glm)
target_compile_definitions(glm INTERFACE
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RADIANS
    GLM_FORCE_AVX2
)

FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG        master
    GIT_PROGRESS   TRUE
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(vma)
if (NOT vma_POPULATED)
    set(VMA_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
    set(VMA_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Populate(vma)
    add_subdirectory(${vma_SOURCE_DIR} ${vma_BINARY_DIR})

    add_library(vma STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/Implementation.cpp
    )
    target_include_directories(vma PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        ${vma_SOURCE_DIR}/include
    )
    target_link_libraries(vma PRIVATE volk)
    target_compile_options(vma PRIVATE -Wno-everything)
endif()

FetchContent_Declare(
    spirv_cross
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross
    GIT_TAG        main
    GIT_PROGRESS   TRUE
    GIT_SHALLOW    TRUE
)
set(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spirv_cross)

FetchContent_Declare(
    mio
    GIT_REPOSITORY https://github.com/vimpunk/mio
    GIT_TAG        master
    GIT_PROGRESS   TRUE
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(mio)

add_library(RetinaDependencies INTERFACE)
target_link_libraries(RetinaDependencies INTERFACE
    TracyClient
    Vulkan::shaderc_combined
    spirv-cross-glsl
    spdlog
    glfw
    volk
    glm
    vma
    mio
)
target_include_directories(RetinaDependencies INTERFACE ${Vulkan_INCLUDE_DIRS})
