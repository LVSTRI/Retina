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
    glm::mat4 InvProjection = {};
    glm::mat4 InvView = {};
    glm::mat4 InvProjView = {};
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
            .Flags = Retina::EAccelerationStructureBuildFlag::E_PREFER_FAST_TRACE_KHR |
                     Retina::EAccelerationStructureBuildFlag::E_ALLOW_DATA_ACCESS_KHR,
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
                    .Transform = Retina::ToNativeTransformMatrix(glm::mat4(1.0f)),
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

    auto mainPipeline = Retina::CRayTracingPipeline::Make(*device, {
        .Name = "MainPipeline",
        .RayGenShader = RETINA_MAIN_SHADER_PATH "/RayTracing/Triangle.rgen.glsl",
        .HitGroupShaders = {
            {
                .ClosestHit = RETINA_MAIN_SHADER_PATH "/RayTracing/Triangle.rchit.glsl",
            },
        },
        .MissShaders = {
            RETINA_MAIN_SHADER_PATH "/RayTracing/Triangle.rmiss.glsl",
        },
        .DescriptorLayouts = { {
            shaderResourceTable->GetDescriptorLayout()
        } },
    });
    auto tonemapImage = Retina::CImage::Make(*device, {
        .Name = "TonemapImage",
        .Width = swapchain->GetWidth(),
        .Height = swapchain->GetHeight(),
        .Usage = Retina::EImageUsage::E_COLOR_ATTACHMENT |
                 Retina::EImageUsage::E_TRANSFER_SRC,
        .Format = Retina::EResourceFormat::E_R8G8B8A8_UNORM,
        .ViewInfo = Retina::Constant::DEFAULT_IMAGE_VIEW_INFO,
    });
    auto tonemapPipeline = Retina::CGraphicsPipeline::Make(*device, {
        .Name = "TonemapPipeline",
        .VertexShader = RETINA_MAIN_SHADER_PATH "/FullscreenTriangle.vert.glsl",
        .FragmentShader = RETINA_MAIN_SHADER_PATH "/Tonemap.frag.glsl",
        .DescriptorLayouts = { {
            shaderResourceTable->GetDescriptorLayout()
        } },
        .DynamicState = { {
            Retina::EDynamicState::E_VIEWPORT,
            Retina::EDynamicState::E_SCISSOR,
        } },
        .RenderingInfo = { {
            .ColorAttachmentFormats = {
                tonemapImage->GetFormat()
            }
        } }
    });
    auto mainImage = shaderResourceTable->MakeStorageImage({
        .Name = "MainImage",
        .Width = swapchain->GetWidth(),
        .Height = swapchain->GetHeight(),
        .Usage = Retina::EImageUsage::E_STORAGE,
        .Format = Retina::EResourceFormat::E_R32G32B32A32_SFLOAT,
        .ViewInfo = Retina::Constant::DEFAULT_IMAGE_VIEW_INFO,
    });
    auto cameraBuffers = shaderResourceTable->MakeUniformBuffer<SCamera>(2, {
        .Name = "CameraBuffer",
        .Heap = Retina::EMemoryProperty::E_DEVICE_MAPPABLE,
        .Capacity = 1,
    });

    while (window->IsOpen()) {
        const auto currentFrameIndex = frameTimeline->WaitForNextTimelineValue();
        shaderResourceTable->Update();
        if (!swapchain->AcquireNextImage(*imageAvailableSemaphores[currentFrameIndex])) {
            continue;
        }
        const auto& swapchainImage = swapchain->GetCurrentImage();

        auto& currentCameraBuffer = cameraBuffers[currentFrameIndex];

        {
            const auto aspectRatio =
                static_cast<float32>(swapchain->GetWidth()) /
                static_cast<float32>(swapchain->GetHeight());
            auto camera = SCamera();
            camera.InvProjection = glm::inverse(glm::infinitePerspective(glm::radians(60.0f), aspectRatio, 0.1f));
            camera.InvView = glm::inverse(glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(), glm::vec3(0.0f, 1.0f, 0.0f)));
            camera.InvProjView = camera.InvProjection * camera.InvView;
            camera.Position = glm::vec4(0.0f, 0.0f, -2.0f, 1.0f);
            currentCameraBuffer->Write(camera);
        }

        auto& currentCommandBuffer = *commandBuffers[currentFrameIndex];
        currentCommandBuffer.GetCommandPool().Reset();
        currentCommandBuffer
            .Begin()
            .ImageMemoryBarrier({
                .Image = *mainImage,
                .SourceStage = Retina::EPipelineStage::E_NONE,
                .DestStage = Retina::EPipelineStage::E_RAY_TRACING_SHADER_KHR,
                .SourceAccess = Retina::EResourceAccess::E_NONE,
                .DestAccess = Retina::EResourceAccess::E_SHADER_STORAGE_WRITE,
                .OldLayout = Retina::EImageLayout::E_UNDEFINED,
                .NewLayout = Retina::EImageLayout::E_GENERAL,
            })
            .BindPipeline(*mainPipeline)
            .BindDescriptorSet(shaderResourceTable->GetDescriptorSet())
            .PushConstants(
                mainImage.GetHandle(),
                currentCameraBuffer.GetHandle(),
                tlas.GetHandle()
            )
            .TraceRays(swapchain->GetWidth(), swapchain->GetHeight(), 1)
            .ImageMemoryBarrier({
                .Image = *mainImage,
                .SourceStage = Retina::EPipelineStage::E_RAY_TRACING_SHADER_KHR,
                .DestStage = Retina::EPipelineStage::E_FRAGMENT_SHADER,
                .SourceAccess = Retina::EResourceAccess::E_SHADER_STORAGE_WRITE,
                .DestAccess = Retina::EResourceAccess::E_SHADER_STORAGE_READ,
                .OldLayout = Retina::EImageLayout::E_GENERAL,
                .NewLayout = Retina::EImageLayout::E_GENERAL,
            })
            .ImageMemoryBarrier({
                .Image = *tonemapImage,
                .SourceStage = Retina::EPipelineStage::E_NONE,
                .DestStage = Retina::EPipelineStage::E_COLOR_ATTACHMENT_OUTPUT,
                .SourceAccess = Retina::EResourceAccess::E_NONE,
                .DestAccess = Retina::EResourceAccess::E_COLOR_ATTACHMENT_WRITE,
                .OldLayout = Retina::EImageLayout::E_UNDEFINED,
                .NewLayout = Retina::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
            })
            .BeginRendering({
                .Name = "TonemapPass",
                .ColorAttachments = {
                    {
                        .Image = *tonemapImage,
                        .LoadOperator = Retina::EAttachmentLoadOperator::E_DONT_CARE,
                        .StoreOperator = Retina::EAttachmentStoreOperator::E_STORE,
                    }
                },
            })
            .SetViewport()
            .SetScissor()
            .BindPipeline(*tonemapPipeline)
            .BindDescriptorSet(shaderResourceTable->GetDescriptorSet())
            .PushConstants(
                mainImage.GetHandle()
            )
            .Draw(3, 1, 0, 0)
            .EndRendering()
            .ImageMemoryBarrier({
                .Image = *tonemapImage,
                .SourceStage = Retina::EPipelineStage::E_COLOR_ATTACHMENT_OUTPUT,
                .DestStage = Retina::EPipelineStage::E_BLIT,
                .SourceAccess = Retina::EResourceAccess::E_COLOR_ATTACHMENT_WRITE,
                .DestAccess = Retina::EResourceAccess::E_TRANSFER_READ,
                .OldLayout = Retina::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
                .NewLayout = Retina::EImageLayout::E_TRANSFER_SRC_OPTIMAL,
            })
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
            .BlitImage(*tonemapImage, swapchainImage, {})
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
