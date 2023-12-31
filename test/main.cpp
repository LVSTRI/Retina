#include <Retina/Core/Core.hpp>
#include <Retina/Graphics/Graphics.hpp>
#include <Retina/Platform/Platform.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define FRAMES_IN_FLIGHT 2

using namespace Retina::Types;
using namespace Retina::Literals;

struct SCamera {
    glm::mat4 Projection = {};
    glm::mat4 View = {};
    glm::mat4 ProjView = {};
    glm::vec4 Position = {};
};

int main() {
    auto window = Retina::CWindow::Make({
        .Title = "Retina Engine",
        .Width = 1280,
        .Height = 720,
        .Features = {
            .Resizable = true,
            .Decorated = true,
            .Focused = true,
        },
    });
    auto instance = Retina::CInstance::Make({
        .Features = {
            .Surface = true,
            .Debug = true,
        },
        .PlatformGetSurfaceExtensionsFunc = Retina::Platform::GetSurfaceExtensions,
    });
    auto device = Retina::CDevice::Make(*instance, {
        .Name = "MainDevice",
        .Extensions = {
            .Swapchain = true,
            .RayTracing = true,
        },
        .Features = {},
    });
    auto swapchain = Retina::CSwapchain::Make(*device, *window, {
        .Name = "MainSwapchain",
        .VSync = true,
        .MakeSurfaceFunc = Retina::Platform::MakeNativeSurface,
    });
    auto commandBuffers = Retina::CCommandBuffer::Make(device->GetGraphicsQueue(), FRAMES_IN_FLIGHT, {
        .Name = "MainCommandBuffer",
        .Primary = true,
        .CommandPoolInfo = Retina::Constant::DEFAULT_COMMAND_POOL_INFO
    });
    auto imageAvailableSemaphores = Retina::CBinarySemaphore::Make(*device, FRAMES_IN_FLIGHT, {
        .Name = "ImageAvailableSemaphore",
    });
    auto presentReadySemaphores = Retina::CBinarySemaphore::Make(*device, FRAMES_IN_FLIGHT, {
        .Name = "RenderFinishedSemaphore",
    });
    auto frameTimeline = Retina::CSyncHostDeviceTimeline::Make(*device, FRAMES_IN_FLIGHT);

    auto shaderResourceTable = Retina::CShaderResourceTable::Make(*device);
    auto blas = [&] {
        auto vertexBuffer = Retina::CTypedBuffer<glm::vec3>::Upload(*device, {
            .Name = "TriangleVertexBuffer",
            .Data = std::to_array({
                glm::vec3(-0.5f,  0.5f, 0.0f),
                glm::vec3( 0.5f,  0.5f, 0.0f),
                glm::vec3( 0.0f, -0.5f, 0.0f),
            })
        });
        auto indexBuffer = Retina::CTypedBuffer<uint32>::Upload(*device, {
            .Name = "TriangleIndexBuffer",
            .Data = std::to_array<uint32>({
                0, 1, 2,
            })
        });
        return Retina::CBottomLevelAccelerationStructure::MakeCompact(*device, {
            .Name = "BLAS_Triangle",
            .Flags = Retina::EAccelerationStructureBuildFlag::E_PREFER_FAST_TRACE_KHR,
            .GeometryInfos = {
                {
                    .Range = {
                        .PrimitiveCount = 3,
                    },
                    .Flags = Retina::EAccelerationStructureGeometryFlag::E_OPAQUE_KHR |
                             Retina::EAccelerationStructureGeometryFlag::E_NO_DUPLICATE_ANY_HIT_INVOCATION_KHR,
                    .Data = Retina::SAccelerationStructureGeometryTrianglesData {
                        .PositionBuffer = vertexBuffer.Get(),
                        .IndexBuffer = indexBuffer.Get(),
                    },
                }
            }
        });
    }();

    auto tlas = [&] {
        auto instanceBuffer = Retina::CTypedBuffer<Retina::SAccelerationStructureGeometryInstance>::Upload(*device, {
            .Name = "TLAS_InstanceBuffer",
            .Data = std::to_array<Retina::SAccelerationStructureGeometryInstance>({
                {
                    .Transform = Retina::ToNativeTransformMatrix(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.0f, 3.0f))),
                    .AccelerationStructureAddress = blas->GetAddress(),
                }
            })
        });
        return shaderResourceTable->MakeAccelerationStructure({
            .Name = "TLAS_Triangle",
            .Flags = Retina::EAccelerationStructureBuildFlag::E_PREFER_FAST_TRACE_KHR,
            .GeometryInfos = {
                {
                    .Range = {
                        .PrimitiveCount = 1,
                    },
                    .Flags = Retina::EAccelerationStructureGeometryFlag::E_OPAQUE_KHR,
                    .Data = Retina::SAccelerationStructureGeometryInstancesData {
                        .InstanceBuffer = instanceBuffer.Get(),
                    },
                }
            }
        });
    }();

    while (window->IsOpen()) {
        const auto currentFrameIndex = frameTimeline->WaitForNextTimelineValue();
        shaderResourceTable->Update();
        if (!swapchain->AcquireNextImage(*imageAvailableSemaphores[currentFrameIndex])) {
            continue;
        }
        const auto& swapchainImage = swapchain->GetCurrentImage();

        auto& currentCommandBuffer = *commandBuffers[currentFrameIndex];
        currentCommandBuffer.GetCommandPool().Reset();
        currentCommandBuffer
            .Begin()
            .BeginNamedRegion("SwapchainCopy")
            .ImageMemoryBarrier({
                .Image = swapchainImage,
                .SourceStage = Retina::EPipelineStage::E_TOP_OF_PIPE,
                .DestStage = Retina::EPipelineStage::E_TRANSFER,
                .SourceAccess = Retina::EResourceAccess::E_NONE,
                .DestAccess = Retina::EResourceAccess::E_TRANSFER_WRITE,
                .OldLayout = Retina::EImageLayout::E_UNDEFINED,
                .NewLayout = Retina::EImageLayout::E_TRANSFER_DST_OPTIMAL,
            })
            .ImageMemoryBarrier({
                .Image = swapchainImage,
                .SourceStage = Retina::EPipelineStage::E_TRANSFER,
                .DestStage = Retina::EPipelineStage::E_BOTTOM_OF_PIPE,
                .SourceAccess = Retina::EResourceAccess::E_TRANSFER_WRITE,
                .DestAccess = Retina::EResourceAccess::E_NONE,
                .OldLayout = Retina::EImageLayout::E_TRANSFER_DST_OPTIMAL,
                .NewLayout = Retina::EImageLayout::E_PRESENT_SRC_KHR,
            })
            .EndNamedRegion()
            .End();

        device->GetGraphicsQueue().Submit({
            .CommandBuffers = { currentCommandBuffer },
            .WaitSemaphores = {
                { *imageAvailableSemaphores[currentFrameIndex], Retina::EPipelineStage::E_TRANSFER },
            },
            .SignalSemaphores = {
                { *presentReadySemaphores[currentFrameIndex], Retina::EPipelineStage::E_TRANSFER },
            },
            .Timeline = *frameTimeline,
        });

        {
            const auto success = device->GetGraphicsQueue().Present({
                .Swapchain = *swapchain,
                .WaitSemaphores = {
                    *presentReadySemaphores[currentFrameIndex],
                },
            });
            if (!success) {
                continue;
            }
        }
        Retina::Platform::PollEvents();
        RETINA_MARK_FRAME();
    }
    device->GetGraphicsQueue().WaitIdle();
    return 0;
}
