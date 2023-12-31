cmake_minimum_required(VERSION 3.26)
project(Retina)
set(CMAKE_CXX_STANDARD 23)

option(RETINA_BUILD_TEST_APPLICATION "Enables Vulkan Beta Extensions" ON)
option(RETINA_ENABLE_DEBUG_LOGGER "Enables internal Debug Logger" ON)
option(RETINA_ENABLE_DEBUG_PROFILER "Enables profiling with Tracy" OFF)
option(RETINA_ENABLE_VULKAN_BETA_EXTENSIONS "Enables Vulkan Beta Extensions" OFF)
option(RETINA_ENABLE_NVIDIA_DLSS "Enables support for NVIDIA DLSS" OFF)

add_subdirectory(dependencies)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "MSVC is not supported")
endif()

set(RETINA_HEADERS
    include/Retina/Core/External/HashMap.hpp

    include/Retina/Core/ArcPtr.hpp
    include/Retina/Core/Core.hpp
    include/Retina/Core/EnableIntrusiveReferenceCount.hpp
    include/Retina/Core/Enums.hpp
    include/Retina/Core/Forwards.hpp
    include/Retina/Core/Macros.hpp
    include/Retina/Core/Meta.hpp
    include/Retina/Core/Types.hpp
    include/Retina/Core/Utility.hpp

    include/Retina/Graphics/Native/NativeDebugName.hpp

    include/Retina/Graphics/RayTracing/AccelerationStructure.hpp
    include/Retina/Graphics/RayTracing/AccelerationStructureInfo.hpp
    include/Retina/Graphics/RayTracing/BottomLevelAccelerationStructure.hpp
    include/Retina/Graphics/RayTracing/RayTracingPipeline.hpp
    include/Retina/Graphics/RayTracing/TopLevelAccelerationStructure.hpp

    include/Retina/Graphics/Resources/DescriptorTable.hpp
    include/Retina/Graphics/Resources/ShaderResourceTable.hpp

    include/Retina/Graphics/Sync/SyncHostTimeline.hpp

    include/Retina/Graphics/BinarySemaphore.hpp
    include/Retina/Graphics/Buffer.hpp
    include/Retina/Graphics/BufferInfo.hpp
    include/Retina/Graphics/CommandBuffer.hpp
    include/Retina/Graphics/CommandBufferInfo.hpp
    include/Retina/Graphics/CommandPool.hpp
    include/Retina/Graphics/CommandPoolInfo.hpp
    include/Retina/Graphics/ComputePipeline.hpp
    include/Retina/Graphics/DescriptorLayout.hpp
    include/Retina/Graphics/DescriptorLayoutInfo.hpp
    include/Retina/Graphics/DescriptorPool.hpp
    include/Retina/Graphics/DescriptorPoolInfo.hpp
    include/Retina/Graphics/DescriptorSet.hpp
    include/Retina/Graphics/DescriptorSetInfo.hpp
    include/Retina/Graphics/Device.hpp
    include/Retina/Graphics/DeviceInfo.hpp
    include/Retina/Graphics/Fence.hpp
    include/Retina/Graphics/FenceInfo.hpp
    include/Retina/Graphics/Graphics.hpp
    include/Retina/Graphics/GraphicsPipeline.hpp
    include/Retina/Graphics/Image.hpp
    include/Retina/Graphics/ImageInfo.hpp
    include/Retina/Graphics/ImageView.hpp
    include/Retina/Graphics/Instance.hpp
    include/Retina/Graphics/InstanceInfo.hpp
    include/Retina/Graphics/Pipeline.hpp
    include/Retina/Graphics/PipelineInfo.hpp
    include/Retina/Graphics/QueryPool.hpp
    include/Retina/Graphics/QueryPoolInfo.hpp
    include/Retina/Graphics/Queue.hpp
    include/Retina/Graphics/QueueInfo.hpp
    include/Retina/Graphics/Sampler.hpp
    include/Retina/Graphics/SamplerInfo.hpp
    include/Retina/Graphics/Semaphore.hpp
    include/Retina/Graphics/SemaphoreInfo.hpp
    include/Retina/Graphics/Swapchain.hpp
    include/Retina/Graphics/SwapchainInfo.hpp
    include/Retina/Graphics/TimelineSemaphore.hpp
    include/Retina/Graphics/TypedBuffer.hpp

    include/Retina/Platform/Input.hpp
    include/Retina/Platform/InputInfo.hpp
    include/Retina/Platform/Platform.hpp
    include/Retina/Platform/PlatformManager.hpp
    include/Retina/Platform/Window.hpp
    include/Retina/Platform/WindowInfo.hpp
)

set(RETINA_SOURCES
    src/Retina/Graphics/Native/NativeDebugName.cpp

    src/Retina/Graphics/RayTracing/AccelerationStructure.cpp
    src/Retina/Graphics/RayTracing/AccelerationStructureInfo.cpp
    src/Retina/Graphics/RayTracing/BottomLevelAccelerationStructure.cpp
    src/Retina/Graphics/RayTracing/RayTracingPipeline.cpp
    src/Retina/Graphics/RayTracing/TopLevelAccelerationStructure.cpp

    src/Retina/Graphics/Resources/DescriptorTable.cpp
    src/Retina/Graphics/Resources/ShaderResourceTable.cpp

    src/Retina/Graphics/Sync/SyncHostTimeline.cpp

    src/Retina/Graphics/BinarySemaphore.cpp
    src/Retina/Graphics/Buffer.cpp
    src/Retina/Graphics/CommandBuffer.cpp
    src/Retina/Graphics/CommandPool.cpp
    src/Retina/Graphics/ComputePipeline.cpp
    src/Retina/Graphics/DescriptorLayout.cpp
    src/Retina/Graphics/DescriptorPool.cpp
    src/Retina/Graphics/DescriptorSet.cpp
    src/Retina/Graphics/Device.cpp
    src/Retina/Graphics/Fence.cpp
    src/Retina/Graphics/GraphicsPipeline.cpp
    src/Retina/Graphics/Image.cpp
    src/Retina/Graphics/ImageInfo.cpp
    src/Retina/Graphics/ImageView.cpp
    src/Retina/Graphics/Instance.cpp
    src/Retina/Graphics/Pipeline.cpp
    src/Retina/Graphics/QueryPool.cpp
    src/Retina/Graphics/Queue.cpp
    src/Retina/Graphics/Sampler.cpp
    src/Retina/Graphics/Semaphore.cpp
    src/Retina/Graphics/Swapchain.cpp
    src/Retina/Graphics/TimelineSemaphore.cpp

    src/Retina/Platform/Input.cpp
    src/Retina/Platform/PlatformManager.cpp
    src/Retina/Platform/Window.cpp
)

add_library(Retina STATIC
    ${RETINA_SOURCES}
    ${RETINA_HEADERS}
)

target_compile_definitions(Retina PUBLIC
    $<$<CONFIG:Debug>:RETINA_DEBUG>

    $<$<BOOL:${WIN32}>:
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX>

    $<$<BOOL:${RETINA_ENABLE_DEBUG_LOGGER}>:RETINA_DEBUG_LOGGER>
    $<$<BOOL:${RETINA_ENABLE_DEBUG_PROFILER}>:RETINA_DEBUG_PROFILER>
    $<$<BOOL:${RETINA_ENABLE_VULKAN_BETA_EXTENSIONS}>:RETINA_VULKAN_BETA_EXTENSIONS>
    $<$<BOOL:${RETINA_ENABLE_NVIDIA_DLSS}>:RETINA_NVIDIA_DLSS>

    RETINA_MAIN_SHADER_PATH="${CMAKE_CURRENT_SOURCE_DIR}/Shaders"

    VK_NO_PROTOTYPES

    $<$<BOOL:${WIN32}>:RETINA_PLATFORM_WIN32>
    $<$<BOOL:${LINUX}>:RETINA_PLATFORM_LINUX>

    TRACY_ENABLE
)

set(RETINA_COMPILER_WARNING_OPTIONS -Wno-missing-field-initializers)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(RETINA_COMPILER_WARNING_OPTIONS ${RETINA_COMPILER_WARNING_OPTIONS}
        -Wno-language-extension-token
        -Wno-gnu-statement-expression
        -Wno-nullability-extension
    )
endif()
target_compile_options(Retina PRIVATE
    -Wall
    -Wextra
    -pedantic
    ${RETINA_COMPILER_WARNING_OPTIONS}
)

target_include_directories(Retina PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(Retina PUBLIC RetinaDependencies)

if (${RETINA_BUILD_TEST_APPLICATION})
    include(FetchContent)
    FetchContent_Declare(
        cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf
        GIT_TAG        master
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    FetchContent_GetProperties(cgltf)
    if (NOT cgltf_POPULATED)
        FetchContent_Populate(cgltf)

        add_library(cgltf STATIC test/cgltf.cpp)
        target_include_directories(cgltf PUBLIC ${cgltf_SOURCE_DIR})
        target_compile_options(cgltf PRIVATE -Wno-everything)
    endif()

    FetchContent_Declare(
        ktx_software
        GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software
        GIT_TAG        170e75893e3d232a2c4f241afe199256f8abb6f9
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    FetchContent_GetProperties(ktx_software)
    if (NOT ktx_software_POPULATED)
        set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
        set(KTX_FEATURE_TOOLS_CTS OFF CACHE BOOL "" FORCE)
        set(KTX_FEATURE_VK_UPLOAD OFF CACHE BOOL "" FORCE)
        set(KTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "" FORCE)
        FetchContent_Populate(ktx_software)

        add_subdirectory(${ktx_software_SOURCE_DIR} ${ktx_software_BINARY_DIR})
    endif()
    add_executable(RetinaTest
        test/main.cpp
    )
    target_link_libraries(RetinaTest PRIVATE Retina cgltf ktx_read)

    add_custom_command(TARGET RetinaTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:ktx_read>
            $<TARGET_FILE_DIR:RetinaTest>
    )
endif()
