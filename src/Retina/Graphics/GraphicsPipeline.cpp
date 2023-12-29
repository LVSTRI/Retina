#include <Retina/Graphics/DescriptorLayout.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/GraphicsPipeline.hpp>

#include <spirv_glsl.hpp>

#include <volk.h>

#include <array>

namespace Retina {
    namespace Spvc = spirv_cross;

    CGraphicsPipeline::CGraphicsPipeline() noexcept : IPipeline(EPipelineType::E_GRAPHICS) {
        RETINA_PROFILE_SCOPED();
    }

    auto CGraphicsPipeline::Make(
        const CDevice& device,
        const SGraphicsPipelineCreateInfo& createInfo
    ) noexcept -> CArcPtr<CGraphicsPipeline> {
        RETINA_PROFILE_SCOPED();
        auto pipeline = CArcPtr(new Self());
        const auto vertexShaderBinary = CompileShaderFromSource(
            device,
            createInfo.VertexShader,
            createInfo.ShaderIncludePaths,
            EShaderStage::E_VERTEX
        );
        auto shaderStages = std::vector<VkPipelineShaderStageCreateInfo>();
        shaderStages.reserve(2);

        const auto vertexShaderCompiler = std::make_unique<Spvc::CompilerGLSL>(vertexShaderBinary);
        shaderStages.push_back({
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = MakeShaderModule(device, vertexShaderBinary),
            .pName = "main",
            .pSpecializationInfo = nullptr,
        });

        auto fragmentShaderCompiler = std::unique_ptr<Spvc::CompilerGLSL>();
        if (createInfo.FragmentShader) {
            const auto fragmentShaderBinary = CompileShaderFromSource(
                device,
                *createInfo.FragmentShader,
                createInfo.ShaderIncludePaths,
                EShaderStage::E_FRAGMENT
            );
            fragmentShaderCompiler = std::make_unique<Spvc::CompilerGLSL>(fragmentShaderBinary);
            shaderStages.push_back({
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = {},
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = MakeShaderModule(device, fragmentShaderBinary),
                .pName = "main",
                .pSpecializationInfo = nullptr,
            });
        }

        auto vertexInputStateCreateInfo = VkPipelineVertexInputStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
        auto inputAssemblyStateCreateInfo = VkPipelineInputAssemblyStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
        inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        auto tessellationStateCreateInfo = VkPipelineTessellationStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO);
        tessellationStateCreateInfo.patchControlPoints = createInfo.TessellationState.PatchControlPoints;

        auto viewports = std::vector<VkViewport>();
        auto scissors = std::vector<VkRect2D>();
        const auto& [viewportInfos, scissorInfos] = createInfo.ViewportState;
        viewports.reserve(viewportInfos.size());
        for (const auto& viewportInfo : viewportInfos) {
            viewports.push_back({
                .x = viewportInfo.X,
                .y = viewportInfo.Y,
                .width = viewportInfo.Width,
                .height = viewportInfo.Height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f
            });
        }
        scissors.reserve(scissorInfos.size());
        for (const auto& scissorInfo : scissorInfos) {
            scissors.push_back({
                .offset = {
                    .x = scissorInfo.X,
                    .y = scissorInfo.Y
                },
                .extent = {
                    .width = scissorInfo.Width,
                    .height = scissorInfo.Height
                }
            });
        }

        auto viewportStateCreateInfo = VkPipelineViewportStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
        viewportStateCreateInfo.viewportCount = viewports.size();
        viewportStateCreateInfo.pViewports = viewports.data();
        viewportStateCreateInfo.scissorCount = scissors.size();
        viewportStateCreateInfo.pScissors = scissors.data();

        auto rasterizationStateCreateInfo = VkPipelineRasterizationStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
        rasterizationStateCreateInfo.depthClampEnable = createInfo.RasterizationState.DepthClampEnable;
        rasterizationStateCreateInfo.rasterizerDiscardEnable = createInfo.RasterizationState.RasterizerDiscardEnable;
        rasterizationStateCreateInfo.polygonMode = ToEnumCounterpart(createInfo.RasterizationState.PolygonMode);
        rasterizationStateCreateInfo.cullMode = ToEnumCounterpart(createInfo.RasterizationState.CullMode);
        rasterizationStateCreateInfo.frontFace = ToEnumCounterpart(createInfo.RasterizationState.FrontFace);
        rasterizationStateCreateInfo.depthBiasEnable = createInfo.RasterizationState.DepthBiasEnable;
        rasterizationStateCreateInfo.depthBiasConstantFactor = createInfo.RasterizationState.DepthBiasConstantFactor;
        rasterizationStateCreateInfo.depthBiasClamp = createInfo.RasterizationState.DepthBiasClamp;
        rasterizationStateCreateInfo.depthBiasSlopeFactor = createInfo.RasterizationState.DepthBiasSlopeFactor;
        rasterizationStateCreateInfo.lineWidth = createInfo.RasterizationState.LineWidth;

        auto multisampleStateCreateInfo = VkPipelineMultisampleStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
        multisampleStateCreateInfo.rasterizationSamples = ToEnumCounterpart(createInfo.MultisampleState.SampleCount);
        multisampleStateCreateInfo.sampleShadingEnable = createInfo.MultisampleState.SampleShadingEnable;
        multisampleStateCreateInfo.minSampleShading = createInfo.MultisampleState.MinSampleShading;
        multisampleStateCreateInfo.pSampleMask = nullptr;
        multisampleStateCreateInfo.alphaToCoverageEnable = createInfo.MultisampleState.AlphaToCoverageEnable;
        multisampleStateCreateInfo.alphaToOneEnable = createInfo.MultisampleState.AlphaToOneEnable;

        auto depthStencilStateCreateInfo = VkPipelineDepthStencilStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
        depthStencilStateCreateInfo.depthTestEnable = createInfo.DepthStencilState.DepthTestEnable;
        depthStencilStateCreateInfo.depthWriteEnable = createInfo.DepthStencilState.DepthWriteEnable;
        depthStencilStateCreateInfo.depthCompareOp = ToEnumCounterpart(createInfo.DepthStencilState.DepthCompareOperator);
        depthStencilStateCreateInfo.depthBoundsTestEnable = createInfo.DepthStencilState.DepthBoundsTestEnable;
        depthStencilStateCreateInfo.stencilTestEnable = createInfo.DepthStencilState.StencilTestEnable;
        depthStencilStateCreateInfo.front = {
            .failOp = ToEnumCounterpart(createInfo.DepthStencilState.FrontStencilState.FailOperator),
            .passOp = ToEnumCounterpart(createInfo.DepthStencilState.FrontStencilState.PassOperator),
            .depthFailOp = ToEnumCounterpart(createInfo.DepthStencilState.FrontStencilState.DepthFailOperator),
            .compareOp = ToEnumCounterpart(createInfo.DepthStencilState.FrontStencilState.CompareOperator),
            .compareMask = createInfo.DepthStencilState.FrontStencilState.CompareMask,
            .writeMask = createInfo.DepthStencilState.FrontStencilState.WriteMask,
            .reference = createInfo.DepthStencilState.FrontStencilState.Reference
        };
        depthStencilStateCreateInfo.back = {
            .failOp = ToEnumCounterpart(createInfo.DepthStencilState.BackStencilState.FailOperator),
            .passOp = ToEnumCounterpart(createInfo.DepthStencilState.BackStencilState.PassOperator),
            .depthFailOp = ToEnumCounterpart(createInfo.DepthStencilState.BackStencilState.DepthFailOperator),
            .compareOp = ToEnumCounterpart(createInfo.DepthStencilState.BackStencilState.CompareOperator),
            .compareMask = createInfo.DepthStencilState.BackStencilState.CompareMask,
            .writeMask = createInfo.DepthStencilState.BackStencilState.WriteMask,
            .reference = createInfo.DepthStencilState.BackStencilState.Reference
        };
        depthStencilStateCreateInfo.minDepthBounds = createInfo.DepthStencilState.MinDepthBounds;
        depthStencilStateCreateInfo.maxDepthBounds = createInfo.DepthStencilState.MaxDepthBounds;

        auto colorBlendAttachments = createInfo.ColorBlendState.Attachments;
        if (fragmentShaderCompiler && colorBlendAttachments.empty()) {
            const auto& resources = fragmentShaderCompiler->get_shader_resources();
            for (const auto& stageOutput : resources.stage_outputs) {
                const auto& type = fragmentShaderCompiler->get_type(stageOutput.base_type_id);
                const auto colorWriteMask = [&] {
                    auto mask = EColorComponent();
                    switch (type.vecsize) {
                        case 4: mask |= EColorComponent::E_A; RETINA_FALLTHROUGH;
                        case 3: mask |= EColorComponent::E_B; RETINA_FALLTHROUGH;
                        case 2: mask |= EColorComponent::E_G; RETINA_FALLTHROUGH;
                        case 1: mask |= EColorComponent::E_R; break;
                        default: std::unreachable();
                    }
                    return mask;
                }();
                colorBlendAttachments.emplace_back(SPipelineColorBlendAttachmentInfo {
                    .ColorWriteMask = colorWriteMask
                });
            }
        }
        auto colorBlendAttachmentStates = std::vector<VkPipelineColorBlendAttachmentState>();
        colorBlendAttachmentStates.reserve(colorBlendAttachments.size());
        for (const auto& colorBlendAttachment : colorBlendAttachments) {
            colorBlendAttachmentStates.push_back({
                .blendEnable = colorBlendAttachment.BlendEnable,
                .srcColorBlendFactor = ToEnumCounterpart(colorBlendAttachment.SourceColorBlendFactor),
                .dstColorBlendFactor = ToEnumCounterpart(colorBlendAttachment.DestColorBlendFactor),
                .colorBlendOp = ToEnumCounterpart(colorBlendAttachment.ColorBlendOperator),
                .srcAlphaBlendFactor = ToEnumCounterpart(colorBlendAttachment.SourceAlphaBlendFactor),
                .dstAlphaBlendFactor = ToEnumCounterpart(colorBlendAttachment.DestAlphaBlendFactor),
                .alphaBlendOp = ToEnumCounterpart(colorBlendAttachment.AlphaBlendOperator),
                .colorWriteMask = static_cast<VkColorComponentFlags>(ToEnumCounterpart(colorBlendAttachment.ColorWriteMask))
            });
        }

        auto colorBlendStateCreateInfo = VkPipelineColorBlendStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
        colorBlendStateCreateInfo.logicOpEnable = createInfo.ColorBlendState.LogicOperatorEnable;
        colorBlendStateCreateInfo.logicOp = ToEnumCounterpart(createInfo.ColorBlendState.LogicOperator);
        colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentStates.size();
        colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.data();
        colorBlendStateCreateInfo.blendConstants[0] = createInfo.ColorBlendState.BlendConstants[0];
        colorBlendStateCreateInfo.blendConstants[1] = createInfo.ColorBlendState.BlendConstants[1];
        colorBlendStateCreateInfo.blendConstants[2] = createInfo.ColorBlendState.BlendConstants[2];
        colorBlendStateCreateInfo.blendConstants[3] = createInfo.ColorBlendState.BlendConstants[3];

        auto dynamicStates = std::vector<VkDynamicState>();
        dynamicStates.reserve(createInfo.DynamicState.DynamicStates.size());
        for (const auto& dynamicStateEnum : createInfo.DynamicState.DynamicStates) {
            dynamicStates.push_back(ToEnumCounterpart(dynamicStateEnum));
        }
        auto dynamicStateCreateInfo = VkPipelineDynamicStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
        dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        // TODO: Better push constant handling
        // TODO: Eventual DSL Reflection when no layout is given
        auto descriptorLayoutHandles = std::vector<VkDescriptorSetLayout>();
        if (createInfo.DescriptorLayouts) {
            descriptorLayoutHandles = MakeDescriptorLayoutHandles(*createInfo.DescriptorLayouts);
        }

        const auto pushConstantInfo = ReflectPushConstantRange(std::to_array<spirv_cross::CompilerGLSL*>({
            vertexShaderCompiler.get(),
            fragmentShaderCompiler.get()
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

        auto renderingColorAttachmentFormats = std::vector<VkFormat>();
        auto renderingDepthFormat = VkFormat();
        auto renderingStencilFormat = VkFormat();
        auto renderingCreateInfo = VkPipelineRenderingCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO);
        if (createInfo.RenderingInfo) {
            const auto& renderingInfo = *createInfo.RenderingInfo;
            renderingColorAttachmentFormats.reserve(renderingInfo.ColorAttachmentFormats.size());
            for (const auto format : renderingInfo.ColorAttachmentFormats) {
                renderingColorAttachmentFormats.push_back(ToEnumCounterpart(format));
            }
            if (renderingInfo.DepthAttachmentFormat) {
                renderingDepthFormat = ToEnumCounterpart(*renderingInfo.DepthAttachmentFormat);
            }
            if (renderingInfo.StencilAttachmentFormat) {
                renderingStencilFormat = ToEnumCounterpart(*renderingInfo.StencilAttachmentFormat);
            }

            renderingCreateInfo.viewMask = renderingInfo.ViewMask;
            renderingCreateInfo.colorAttachmentCount = renderingColorAttachmentFormats.size();
            renderingCreateInfo.pColorAttachmentFormats = renderingColorAttachmentFormats.data();
            renderingCreateInfo.depthAttachmentFormat = renderingDepthFormat;
            renderingCreateInfo.stencilAttachmentFormat = renderingStencilFormat;
        }

        auto graphicsPipelineCreateInfo = VkGraphicsPipelineCreateInfo(VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
        if (createInfo.RenderingInfo) {
            graphicsPipelineCreateInfo.pNext = &renderingCreateInfo;
        }
        graphicsPipelineCreateInfo.stageCount = shaderStages.size();
        graphicsPipelineCreateInfo.pStages = shaderStages.data();
        graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
        graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
        graphicsPipelineCreateInfo.pTessellationState = &tessellationStateCreateInfo;
        graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
        graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
        graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        graphicsPipelineCreateInfo.layout = pipelineLayoutHandle;
        graphicsPipelineCreateInfo.renderPass = {};
        graphicsPipelineCreateInfo.subpass = 0;
        graphicsPipelineCreateInfo.basePipelineHandle = {};
        graphicsPipelineCreateInfo.basePipelineIndex = -1;
        auto graphicsPipelineHandle = VkPipeline();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkCreateGraphicsPipelines(
                device.GetHandle(),
                {},
                1,
                &graphicsPipelineCreateInfo,
                nullptr,
                &graphicsPipelineHandle
            )
        );
        for (const auto& shaderStage : shaderStages) {
            vkDestroyShaderModule(device.GetHandle(), shaderStage.module, nullptr);
        }

        pipeline->_handle = graphicsPipelineHandle;
        pipeline->_layout = {
            .Handle = pipelineLayoutHandle,
            .PushConstantInfo = pushConstantInfo,
        };
        pipeline->_createInfo = createInfo;
        pipeline->_device = device.ToArcPtr();
        pipeline->SetDebugName(createInfo.Name);

        return pipeline;
    }

    auto CGraphicsPipeline::GetCreateInfo() const noexcept -> const SGraphicsPipelineCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }
}
