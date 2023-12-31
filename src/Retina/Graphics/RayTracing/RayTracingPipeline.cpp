#pragma once

#include <Retina/Graphics/RayTracing/RayTracingPipeline.hpp>
#include <Retina/Graphics/Buffer.hpp>
#include <Retina/Graphics/DescriptorLayout.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/PipelineInfo.hpp>

#include <spirv_glsl.hpp>

#include <volk.h>

#include <array>
#include <cstring>

namespace Retina {
    namespace Spvc = spirv_cross;

    CRayTracingPipeline::CRayTracingPipeline() noexcept : IPipeline(EPipelineType::E_RAY_TRACING) {
        RETINA_PROFILE_SCOPED();
    }

    auto CRayTracingPipeline::Make(
        const CDevice& device,
        const SRayTracingPipelineCreateInfo& createInfo
    ) noexcept -> CArcPtr <Self> {
        RETINA_PROFILE_SCOPED();
        auto pipeline = CArcPtr(new Self());

        // 1. Compile all RayGen Shaders
        auto rayGenShaderStage = VkPipelineShaderStageCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
        auto rayGenShaderCompiler = std::unique_ptr<Spvc::CompilerGLSL>();
        {
            const auto binary = CompileShaderFromSource(
                device,
                createInfo.RayGenShader,
                createInfo.ShaderIncludePaths,
                EShaderStage::E_RAYGEN_KHR
            );
            rayGenShaderStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            rayGenShaderStage.module = MakeShaderModule(device, binary);
            rayGenShaderStage.pName = "main";
            rayGenShaderCompiler = std::make_unique<Spvc::CompilerGLSL>(binary);
        }

        // 2. Compile all Miss Shaders
        auto missShaderStages = std::vector<VkPipelineShaderStageCreateInfo>();
        auto missShaderCompilers = std::vector<std::unique_ptr<Spvc::CompilerGLSL>>();
        for (const auto& shader : createInfo.MissShaders) {
            const auto binary = CompileShaderFromSource(
                device,
                shader,
                createInfo.ShaderIncludePaths,
                EShaderStage::E_MISS_KHR
            );
            auto stageCreateInfo = VkPipelineShaderStageCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
            stageCreateInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
            stageCreateInfo.module = MakeShaderModule(device, binary);
            stageCreateInfo.pName = "main";
            missShaderStages.emplace_back(stageCreateInfo);
            missShaderCompilers.emplace_back(std::make_unique<Spvc::CompilerGLSL>(binary));
        }

        // 3. Compile all Shaders in HitGroups
        struct SHitGroupStages {
            VkPipelineShaderStageCreateInfo ClosestHit = {};
            std::optional<VkPipelineShaderStageCreateInfo> AnyHit = std::nullopt;
            std::optional<VkPipelineShaderStageCreateInfo> Intersection = std::nullopt;
        };
        struct SHitGroupCompilers {
            std::unique_ptr<Spvc::CompilerGLSL> ClosestHit = {};
            std::unique_ptr<Spvc::CompilerGLSL> AnyHit = {};
            std::unique_ptr<Spvc::CompilerGLSL> Intersection = {};
        };
        auto hitGroupShaderStages = std::vector<SHitGroupStages>();
        auto hitGroupShaderCompilers = std::vector<SHitGroupCompilers>();
        for (const auto& [closestHit, anyHit, intersection] : createInfo.HitGroupShaders) {
            auto hitGroupStages = SHitGroupStages();
            auto hitGroupCompilers = SHitGroupCompilers();
            // Closest Hit
            {
                const auto binary = CompileShaderFromSource(
                    device,
                    closestHit,
                    createInfo.ShaderIncludePaths,
                    EShaderStage::E_CLOSEST_HIT_KHR
                );
                auto stageCreateInfo = VkPipelineShaderStageCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
                stageCreateInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
                stageCreateInfo.module = MakeShaderModule(device, binary);
                stageCreateInfo.pName = "main";

                hitGroupStages.ClosestHit = stageCreateInfo;
                hitGroupCompilers.ClosestHit = std::make_unique<Spvc::CompilerGLSL>(binary);
            }

            // Any Hit
            if (anyHit) {
                const auto binary = CompileShaderFromSource(
                    device,
                    *anyHit,
                    createInfo.ShaderIncludePaths,
                    EShaderStage::E_ANY_HIT_KHR
                );
                auto stageCreateInfo = VkPipelineShaderStageCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
                stageCreateInfo.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
                stageCreateInfo.module = MakeShaderModule(device, binary);
                stageCreateInfo.pName = "main";

                hitGroupStages.AnyHit = stageCreateInfo;
                hitGroupCompilers.AnyHit = std::make_unique<Spvc::CompilerGLSL>(binary);
            }

            // Intersection
            if (intersection) {
                const auto binary = CompileShaderFromSource(
                    device,
                    *intersection,
                    createInfo.ShaderIncludePaths,
                    EShaderStage::E_INTERSECTION_KHR
                );
                auto stageCreateInfo = VkPipelineShaderStageCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
                stageCreateInfo.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
                stageCreateInfo.module = MakeShaderModule(device, binary);
                stageCreateInfo.pName = "main";

                hitGroupStages.Intersection = stageCreateInfo;
                hitGroupCompilers.Intersection = std::make_unique<Spvc::CompilerGLSL>(binary);
            }
            hitGroupShaderStages.emplace_back(hitGroupStages);
            hitGroupShaderCompilers.emplace_back(std::move(hitGroupCompilers));
        }

        auto shaderStages = std::vector<VkPipelineShaderStageCreateInfo>();
        shaderStages.reserve(1 + missShaderStages.size() + hitGroupShaderStages.size() * 3);

        auto shaderCompilerHandles = std::vector<const Spvc::CompilerGLSL*>();
        shaderCompilerHandles.reserve(1 + missShaderCompilers.size() + hitGroupShaderCompilers.size() * 3);

        auto rayTracingShaderGroups = std::vector<VkRayTracingShaderGroupCreateInfoKHR>();
        {
            const auto curretRayGenIndex = shaderStages.size();
            auto shaderGroup = VkRayTracingShaderGroupCreateInfoKHR(VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR);
            shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            shaderGroup.generalShader = curretRayGenIndex;
            shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
            rayTracingShaderGroups.emplace_back(shaderGroup);
            shaderStages.emplace_back(rayGenShaderStage);
            shaderCompilerHandles.emplace_back(rayGenShaderCompiler.get());
        }
        for (auto i = 0_u32; i < missShaderStages.size(); ++i) {
            const auto currentMissIndex = shaderStages.size();
            auto shaderGroup = VkRayTracingShaderGroupCreateInfoKHR(VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR);
            shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            shaderGroup.generalShader = currentMissIndex;
            shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
            rayTracingShaderGroups.emplace_back(shaderGroup);
            shaderStages.emplace_back(missShaderStages[i]);
            shaderCompilerHandles.emplace_back(missShaderCompilers[i].get());
        }
        for (auto i = 0_u32; i < hitGroupShaderStages.size(); ++i) {
            const auto currentHitGroupIndex = shaderStages.size();
            auto shaderGroup = VkRayTracingShaderGroupCreateInfoKHR(VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR);
            shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.closestHitShader = currentHitGroupIndex;
            shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
            shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
            shaderStages.emplace_back(hitGroupShaderStages[i].ClosestHit);
            shaderCompilerHandles.emplace_back(hitGroupShaderCompilers[i].ClosestHit.get());
            if (hitGroupShaderStages[i].AnyHit) {
                shaderGroup.anyHitShader = shaderStages.size();
                shaderStages.emplace_back(*hitGroupShaderStages[i].AnyHit);
                shaderCompilerHandles.emplace_back(hitGroupShaderCompilers[i].AnyHit.get());
            }
            if (hitGroupShaderStages[i].Intersection) {
                shaderGroup.intersectionShader = shaderStages.size();
                shaderStages.emplace_back(*hitGroupShaderStages[i].Intersection);
                shaderCompilerHandles.emplace_back(hitGroupShaderCompilers[i].Intersection.get());
            }
            rayTracingShaderGroups.emplace_back(shaderGroup);
        }

        auto dynamicStates = std::vector<VkDynamicState>();
        dynamicStates.reserve(createInfo.DynamicState.DynamicStates.size());
        for (const auto& dynamicStateEnum : createInfo.DynamicState.DynamicStates) {
            dynamicStates.push_back(ToEnumCounterpart(dynamicStateEnum));
        }
        auto dynamicStateCreateInfo = VkPipelineDynamicStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
        dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        auto descriptorLayoutHandles = std::vector<VkDescriptorSetLayout>();
        if (createInfo.DescriptorLayouts) {
            descriptorLayoutHandles = MakeDescriptorLayoutHandles(*createInfo.DescriptorLayouts);
        }
        const auto pushConstantInfo = ReflectPushConstantRange(shaderCompilerHandles);
        const auto nativePushConstantInfo = std::bit_cast<VkPushConstantRange>(pushConstantInfo);

        auto pipelineLayoutCreateInfo = VkPipelineLayoutCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
        pipelineLayoutCreateInfo.setLayoutCount = descriptorLayoutHandles.size();
        pipelineLayoutCreateInfo.pSetLayouts = descriptorLayoutHandles.data();
        if (pushConstantInfo.Size > 0) {
            pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
            pipelineLayoutCreateInfo.pPushConstantRanges = &nativePushConstantInfo;
        }
        auto pipelineLayoutHandle = VkPipelineLayout();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkCreatePipelineLayout(
                device.GetHandle(),
                &pipelineLayoutCreateInfo,
                nullptr,
                &pipelineLayoutHandle
            )
        );

        auto rayTracingPipelineCreateInfo = VkRayTracingPipelineCreateInfoKHR(VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR);
        rayTracingPipelineCreateInfo.stageCount = shaderStages.size();
        rayTracingPipelineCreateInfo.pStages = shaderStages.data();
        rayTracingPipelineCreateInfo.groupCount = rayTracingShaderGroups.size();
        rayTracingPipelineCreateInfo.pGroups = rayTracingShaderGroups.data();
        rayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
        rayTracingPipelineCreateInfo.pLibraryInfo = nullptr;
        rayTracingPipelineCreateInfo.pLibraryInterface = nullptr;
        rayTracingPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        rayTracingPipelineCreateInfo.layout = pipelineLayoutHandle;
        rayTracingPipelineCreateInfo.basePipelineHandle = {};
        rayTracingPipelineCreateInfo.basePipelineIndex = -1;

        auto rayTracingPipelineHandle = VkPipeline();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkCreateRayTracingPipelinesKHR(
                device.GetHandle(),
                {},
                {},
                1,
                &rayTracingPipelineCreateInfo,
                nullptr,
                &rayTracingPipelineHandle
            )
        );

        for (const auto& shaderStage : shaderStages) {
            vkDestroyShaderModule(device.GetHandle(), shaderStage.module, nullptr);
        }

        const auto shaderHandleSize = device.GetRayTracingProperty(&SDeviceRayTracingProperties::ShaderGroupHandleSize);
        const auto shaderHandleAlignment = device.GetRayTracingProperty(&SDeviceRayTracingProperties::ShaderGroupHandleAlignment);
        const auto shaderHandleBaseAlignment = device.GetRayTracingProperty(&SDeviceRayTracingProperties::ShaderGroupBaseAlignment);
        const auto shaderHandleAlignedSize = MakeAlignedSize(shaderHandleSize, shaderHandleAlignment);
        const auto shaderHandleBaseAlignedSize = MakeAlignedSize(shaderHandleSize, shaderHandleBaseAlignment);
        const auto shaderGroupCount = rayTracingShaderGroups.size();
        auto shaderGroupHandles = std::vector<uint8>(shaderGroupCount * shaderHandleSize);
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkGetRayTracingShaderGroupHandlesKHR(
                device.GetHandle(),
                rayTracingPipelineHandle,
                0,
                shaderGroupCount,
                shaderGroupHandles.size(),
                shaderGroupHandles.data()
            )
        );
        auto rayGenShaderRegion = SShaderBindingTableRegion();
        rayGenShaderRegion.Stride = shaderHandleBaseAlignedSize;
        rayGenShaderRegion.Size = rayGenShaderRegion.Stride;

        auto missShaderRegion = SShaderBindingTableRegion();
        missShaderRegion.Stride = shaderHandleAlignedSize;
        missShaderRegion.Size = MakeAlignedSize(missShaderStages.size() * shaderHandleAlignedSize, shaderHandleBaseAlignment);

        auto hitGroupShaderRegion = SShaderBindingTableRegion();
        hitGroupShaderRegion.Stride = shaderHandleAlignedSize;
        hitGroupShaderRegion.Size = MakeAlignedSize(hitGroupShaderStages.size() * shaderHandleAlignedSize, shaderHandleBaseAlignment);

        auto shaderBindingTableBuffer = CBuffer::Make(device, {
            .Name = std::format("{}_ShaderBindingTable", createInfo.Name),
            .Heap = EMemoryProperty::E_DEVICE_MAPPABLE,
            .Capacity = rayGenShaderRegion.Size +
                        missShaderRegion.Size +
                        hitGroupShaderRegion.Size,
        });
        shaderBindingTableBuffer->Clear();
        const auto* shaderHandlesBasePtr = shaderGroupHandles.data();
        shaderBindingTableBuffer->Write(shaderHandlesBasePtr, shaderHandleSize, 0);
        for (auto i = 0_u32; i < missShaderStages.size(); ++i) {
            const auto handleOffset = shaderHandleSize * (i + 1);
            const auto currentOffset = rayGenShaderRegion.Size + missShaderRegion.Stride * i;
            shaderBindingTableBuffer->Write(
                shaderHandlesBasePtr + handleOffset,
                shaderHandleSize,
                currentOffset
            );
        }
        for (auto i = 0_u32; i < hitGroupShaderStages.size(); ++i) {
            const auto handleOffset = shaderHandleSize * (i + 1 + missShaderStages.size());
            const auto currentOffset =
                rayGenShaderRegion.Size +
                missShaderRegion.Size +
                hitGroupShaderRegion.Stride * i;
            shaderBindingTableBuffer->Write(
                shaderHandlesBasePtr + handleOffset,
                shaderHandleSize,
                currentOffset
            );
        }

        rayGenShaderRegion.Address = shaderBindingTableBuffer->GetAddress();
        missShaderRegion.Address = rayGenShaderRegion.Address + rayGenShaderRegion.Size;
        hitGroupShaderRegion.Address = missShaderRegion.Address + missShaderRegion.Size;

        pipeline->_handle = rayTracingPipelineHandle;
        pipeline->_layout = {
            .Handle = pipelineLayoutHandle,
            .PushConstantInfo = pushConstantInfo,
        };
        pipeline->_shaderBindingTable = {
            .Buffer = std::move(shaderBindingTableBuffer),
            .Regions = {
                rayGenShaderRegion,
                missShaderRegion,
                hitGroupShaderRegion,
            },
        };
        pipeline->_createInfo = createInfo;
        pipeline->_device = device.ToArcPtr();
        pipeline->SetDebugName(createInfo.Name);

        return pipeline;
    }

    auto CRayTracingPipeline::GetShaderBindingTable() const noexcept -> const SShaderBindingTable& {
        RETINA_PROFILE_SCOPED();
        return _shaderBindingTable;
    }

    auto CRayTracingPipeline::GetCreateInfo() const noexcept -> const SRayTracingPipelineCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }
}
