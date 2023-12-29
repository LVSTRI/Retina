#include <Retina/Graphics/ComputePipeline.hpp>
#include <Retina/Graphics/DescriptorLayout.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Image.hpp>

#include <spirv_glsl.hpp>

#include <volk.h>

#include <array>

namespace Retina {
    namespace Spvc = spirv_cross;

    CComputePipeline::CComputePipeline() noexcept : IPipeline(EPipelineType::E_COMPUTE) {
        RETINA_PROFILE_SCOPED();
    }

    auto CComputePipeline::Make(
        const CDevice& device,
        const SComputePipelineCreateInfo& createInfo
    ) noexcept -> CArcPtr<CComputePipeline> {
        RETINA_PROFILE_SCOPED();
        auto pipeline = CArcPtr(new Self());

        auto computeShaderBinary = CompileShaderFromSource(
            device,
            createInfo.ComputeShader,
            createInfo.ShaderIncludePaths,
            EShaderStage::E_COMPUTE
        );
        auto computeShaderCompiler = std::make_unique<Spvc::CompilerGLSL>(computeShaderBinary);
        auto shaderStage = VkPipelineShaderStageCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
        shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStage.module = MakeShaderModule(device, computeShaderBinary);
        shaderStage.pName = "main";

        // TODO: Eventual DSL Reflection when no layout is given
        auto descriptorLayoutHandles = std::vector<VkDescriptorSetLayout>();
        if (createInfo.DescriptorLayouts) {
            descriptorLayoutHandles = MakeDescriptorLayoutHandles(*createInfo.DescriptorLayouts);
        }

        const auto pushConstantInfo = ReflectPushConstantRange(std::to_array({
            computeShaderCompiler.get()
        }));
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

        auto computePipelineCreateInfo = VkComputePipelineCreateInfo(VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO);
        computePipelineCreateInfo.stage = shaderStage;
        computePipelineCreateInfo.layout = pipelineLayoutHandle;
        computePipelineCreateInfo.basePipelineHandle = {};
        computePipelineCreateInfo.basePipelineIndex = -1;

        auto computePipelineHandle = VkPipeline();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkCreateComputePipelines(
                device.GetHandle(),
                {},
                1,
                &computePipelineCreateInfo,
                nullptr,
                &computePipelineHandle
            )
        );

        pipeline->_handle = computePipelineHandle;
        pipeline->_layout = {
            .Handle = pipelineLayoutHandle,
            .PushConstantInfo = pushConstantInfo,
        };
        pipeline->_createInfo = createInfo;
        pipeline->_device = device.ToArcPtr();
        pipeline->SetDebugName(createInfo.Name);

        return pipeline;
    }

    auto CComputePipeline::GetCreateInfo() const noexcept -> const SComputePipelineCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }
}
