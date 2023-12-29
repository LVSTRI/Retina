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
    auto mainImage = shaderResourceTable->MakeStorageImage({
        .Name = "MainImage",
        .Width = swapchain->GetWidth(),
        .Height = swapchain->GetHeight(),
        .Usage = Retina::EImageUsage::E_COLOR_ATTACHMENT |
                 Retina::EImageUsage::E_STORAGE,
        .Format = Retina::EResourceFormat::E_R32G32B32A32_SFLOAT,
        .ViewInfo = Retina::Constant::DEFAULT_IMAGE_VIEW_INFO,
    });
    auto mainPipeline = Retina::CGraphicsPipeline::Make(*device, {
        .Name = "MainPipeline",
        .VertexShader = "Triangle.vert.glsl",
        .FragmentShader = "Triangle.frag.glsl",
        .DescriptorLayouts = { { shaderResourceTable->GetDescriptorLayout() } },
        .DynamicState = { {
            Retina::EDynamicState::E_VIEWPORT,
            Retina::EDynamicState::E_SCISSOR,
        } },
        .RenderingInfo = { {
            .ColorAttachmentFormats = {
                mainImage->GetFormat()
            }
        } },
    });
    auto finalImage = Retina::CImage::Make(*device, {
        .Name = "FinalImage",
        .Width = swapchain->GetWidth(),
        .Height = swapchain->GetHeight(),
        .Usage = Retina::EImageUsage::E_COLOR_ATTACHMENT |
                 Retina::EImageUsage::E_TRANSFER_SRC,
        .Format = Retina::EResourceFormat::E_R8G8B8A8_UNORM,
        .ViewInfo = Retina::Constant::DEFAULT_IMAGE_VIEW_INFO,
    });
    auto tonemapPipeline = Retina::CGraphicsPipeline::Make(*device, {
        .Name = "TonemapPipeline",
        .VertexShader = "FullscreenTriangle.vert.glsl",
        .FragmentShader = "Tonemap.frag.glsl",
        .DescriptorLayouts = { { shaderResourceTable->GetDescriptorLayout() } },
        .DynamicState = { {
            Retina::EDynamicState::E_VIEWPORT,
            Retina::EDynamicState::E_SCISSOR,
        } },
        .RenderingInfo = { {
            .ColorAttachmentFormats = {
                finalImage->GetFormat()
            }
        } },
    });
    auto cameraBuffers = shaderResourceTable->MakeStorageBuffer<SCamera>(2, {
        .Name = "CameraBuffer",
        .Heap = Retina::Constant::HEAP_TYPE_DEVICE_MAPPABLE,
        .Capacity = 1,
    });
    const auto resize = [&] {
        RETINA_LOG_WARN(device->GetLogger(), "Swapchain Lost, Recreating");
        device->WaitIdle();
        window->UpdateViewportExtent();
        swapchain = Retina::CSwapchain::Recreate(std::move(swapchain));
        mainImage.Destroy();
        mainImage = shaderResourceTable->MakeStorageImage({
            .Name = "MainImage",
            .Width = swapchain->GetWidth(),
            .Height = swapchain->GetHeight(),
            .Usage = Retina::EImageUsage::E_COLOR_ATTACHMENT |
                     Retina::EImageUsage::E_STORAGE,
            .Format = Retina::EResourceFormat::E_R32G32B32A32_SFLOAT,
            .ViewInfo = Retina::Constant::DEFAULT_IMAGE_VIEW_INFO,
        });
        finalImage = Retina::CImage::Make(*device, {
            .Name = "FinalImage",
            .Width = swapchain->GetWidth(),
            .Height = swapchain->GetHeight(),
            .Usage = Retina::EImageUsage::E_COLOR_ATTACHMENT |
                     Retina::EImageUsage::E_TRANSFER_SRC,
            .Format = Retina::EResourceFormat::E_R8G8B8A8_UNORM,
            .ViewInfo = Retina::Constant::DEFAULT_IMAGE_VIEW_INFO,
        });
    };

    while (window->IsOpen()) {
        const auto currentFrameIndex = frameTimeline->WaitForNextTimelineValue();
        shaderResourceTable->Update();

        {
            const auto success = swapchain->AcquireNextImage(*imageAvailableSemaphores[currentFrameIndex]);
            if (!success) {
                resize();
                continue;
            }
        }
        const auto& swapchainImage = swapchain->GetCurrentImage();

        auto& currentCommandBuffer = *commandBuffers[currentFrameIndex];
        auto& currentCameraBuffer = cameraBuffers[currentFrameIndex];

        auto camera = SCamera();
        {
            const auto aspectRatio =
                static_cast<float32>(mainImage->GetWidth()) /
                static_cast<float32>(mainImage->GetHeight());
            camera.Projection = glm::infinitePerspective(glm::radians(60.0f), aspectRatio, 0.1f);
            camera.View = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(), glm::vec3(0.0f, 1.0f, 0.0f));
            camera.ProjView = camera.Projection * camera.View;
            camera.Position = glm::make_vec4(glm::vec3(0.0f, 0.0f, 2.0f));
            currentCameraBuffer->Write(camera);
        }

        currentCommandBuffer.GetCommandPool().Reset();
        currentCommandBuffer
            .Begin()
            .ImageMemoryBarrier({
                .Image = *mainImage,
                .SourceStage = Retina::EPipelineStage::E_TOP_OF_PIPE,
                .DestStage = Retina::EPipelineStage::E_COLOR_ATTACHMENT_OUTPUT,
                .SourceAccess = Retina::EResourceAccess::E_NONE,
                .DestAccess = Retina::EResourceAccess::E_COLOR_ATTACHMENT_WRITE,
                .OldLayout = Retina::EImageLayout::E_UNDEFINED,
                .NewLayout = Retina::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
            })
            .BeginRendering({
                .Name = "MainRenderingPass",
                .ColorAttachments = {
                    {
                        .Image = *mainImage,
                        .LoadOperation = Retina::EAttachmentLoadOperator::E_CLEAR,
                        .StoreOperation = Retina::EAttachmentStoreOperator::E_STORE,
                        .ClearValue = Retina::MakeClearColorValue({ 0.0f, 0.0f, 0.0f, 1.0f }),
                    }
                },
            })
            .SetViewport()
            .SetScissor()
            .BindPipeline(*mainPipeline)
            .BindDescriptorSet(shaderResourceTable->GetDescriptorSet())
            .PushConstants(
                currentCameraBuffer.GetHandle()
            )
            .Draw(3, 1, 0, 0)
            .EndRendering()
            .ImageMemoryBarrier({
                .Image = *mainImage,
                .SourceStage = Retina::EPipelineStage::E_COLOR_ATTACHMENT_OUTPUT,
                .DestStage = Retina::EPipelineStage::E_FRAGMENT_SHADER,
                .SourceAccess = Retina::EResourceAccess::E_COLOR_ATTACHMENT_WRITE,
                .DestAccess = Retina::EResourceAccess::E_SHADER_STORAGE_READ,
                .OldLayout = Retina::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
                .NewLayout = Retina::EImageLayout::E_GENERAL,
            })
            .ImageMemoryBarrier({
                .Image = *finalImage,
                .SourceStage = Retina::EPipelineStage::E_TOP_OF_PIPE,
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
                        .Image = *finalImage,
                        .LoadOperation = Retina::EAttachmentLoadOperator::E_DONT_CARE,
                        .StoreOperation = Retina::EAttachmentStoreOperator::E_STORE,
                    }
                },
            })
            .BindPipeline(*tonemapPipeline)
            .BindDescriptorSet(shaderResourceTable->GetDescriptorSet())
            .PushConstants(
                mainImage.GetHandle()
            )
            .Draw(3, 1, 0, 0)
            .EndRendering()
            .ImageMemoryBarrier({
                .Image = *finalImage,
                .SourceStage = Retina::EPipelineStage::E_COLOR_ATTACHMENT_OUTPUT,
                .DestStage = Retina::EPipelineStage::E_TRANSFER,
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
            .BlitImage(*finalImage, swapchainImage, {})
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
                resize();
                continue;
            }
        }
        Retina::Platform::PollEvents();
        RETINA_MARK_FRAME();
    }
    device->GetGraphicsQueue().WaitIdle();
    return 0;
}
