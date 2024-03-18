#include <Retina/Graphics/DescriptorLayout.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>
#include <Retina/Graphics/MeshShadingPipeline.hpp>

#include <spirv_glsl.hpp>

#include <volk.h>

#include <vector>
#include <array>

namespace Retina::Graphics {
  CMeshShadingPipeline::CMeshShadingPipeline() noexcept
    : IPipeline(EPipelineType::E_MESH_SHADING)
  {
    RETINA_PROFILE_SCOPED();
  }

  CMeshShadingPipeline::~CMeshShadingPipeline() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      RETINA_GRAPHICS_INFO("Mesh shading pipeline ({}) destroyed", GetDebugName());
    }
  }

  auto CMeshShadingPipeline::Make(
    const CDevice& device,
    const SMeshShadingPipelineCreateInfo& createInfo
  ) noexcept -> Core::CArcPtr<CMeshShadingPipeline> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CMeshShadingPipeline());
    auto shaderStages = std::vector<VkPipelineShaderStageCreateInfo>();

    const auto meshShaderBinary = Details::CompileShaderFromSource(
      createInfo.MeshShader,
      createInfo.IncludeDirectories,
      EShaderStageFlag::E_MESH_EXT
    );
    const auto meshShaderCompiler = Core::MakeUnique<spirv_cross::CompilerGLSL>(meshShaderBinary);
    {
      auto stage = VkPipelineShaderStageCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
      stage.stage = VK_SHADER_STAGE_MESH_BIT_EXT;
      stage.module = Details::MakeShaderModule(device, meshShaderBinary);
      stage.pName = "main";
      shaderStages.emplace_back(stage);
    }

    auto taskShaderCompiler = Core::CUniquePtr<spirv_cross::CompilerGLSL>();
    if (createInfo.TaskShader) {
      const auto taskShaderBinary = Details::CompileShaderFromSource(
        createInfo.TaskShader.value(),
        createInfo.IncludeDirectories,
        EShaderStageFlag::E_TASK_EXT
      );
      taskShaderCompiler = Core::MakeUnique<spirv_cross::CompilerGLSL>(taskShaderBinary);
      {
        auto stage = VkPipelineShaderStageCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
        stage.stage = VK_SHADER_STAGE_TASK_BIT_EXT;
        stage.module = Details::MakeShaderModule(device, taskShaderBinary);
        stage.pName = "main";
        shaderStages.emplace_back(stage);
      }
    }

    auto fragmentShaderCompiler = Core::CUniquePtr<spirv_cross::CompilerGLSL>();
    if (createInfo.FragmentShader) {
      const auto fragmentShaderBinary = Details::CompileShaderFromSource(
        createInfo.FragmentShader.value(),
        createInfo.IncludeDirectories,
        EShaderStageFlag::E_FRAGMENT
      );
      fragmentShaderCompiler = Core::MakeUnique<spirv_cross::CompilerGLSL>(fragmentShaderBinary);
      {
        auto stage = VkPipelineShaderStageCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
        stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stage.module = Details::MakeShaderModule(device, fragmentShaderBinary);
        stage.pName = "main";
        shaderStages.emplace_back(stage);
      }
    }

    auto viewports = std::vector<VkViewport>();
    auto scissors = std::vector<VkRect2D>();

    const auto& [viewportInfos, scissorInfos] = createInfo.ViewportState;
    viewports.reserve(viewportInfos.size());
    for (const auto& viewportInfo : viewportInfos) {
      viewports.emplace_back(
        viewportInfo.X,
        viewportInfo.Y,
        viewportInfo.Width,
        viewportInfo.Height,
        0.0f,
        1.0f
      );
    }
    scissors.reserve(scissorInfos.size());
    for (const auto& scissorInfo : scissorInfos) {
      scissors.emplace_back(
        VkOffset2D {
          .x = scissorInfo.X,
          .y = scissorInfo.Y
        },
        VkExtent2D {
          .width = scissorInfo.Width,
          .height = scissorInfo.Height
        }
      );
    }

    auto viewportStateCreateInfo = VkPipelineViewportStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
    viewportStateCreateInfo.viewportCount = viewports.size();
    viewportStateCreateInfo.pViewports = viewports.data();
    viewportStateCreateInfo.scissorCount = scissors.size();
    viewportStateCreateInfo.pScissors = scissors.data();

    auto rasterizationStateCreateInfo = VkPipelineRasterizationStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
    rasterizationStateCreateInfo.depthClampEnable = createInfo.RasterizationState.DepthClampEnable;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = createInfo.RasterizationState.RasterizerDiscardEnable;
    rasterizationStateCreateInfo.polygonMode = AsEnumCounterpart(createInfo.RasterizationState.PolygonMode);
    rasterizationStateCreateInfo.cullMode = AsEnumCounterpart(createInfo.RasterizationState.CullMode);
    rasterizationStateCreateInfo.frontFace = AsEnumCounterpart(createInfo.RasterizationState.FrontFace);
    rasterizationStateCreateInfo.depthBiasEnable = createInfo.RasterizationState.DepthBiasEnable;
    rasterizationStateCreateInfo.depthBiasConstantFactor = createInfo.RasterizationState.DepthBiasConstantFactor;
    rasterizationStateCreateInfo.depthBiasClamp = createInfo.RasterizationState.DepthBiasClamp;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = createInfo.RasterizationState.DepthBiasSlopeFactor;
    rasterizationStateCreateInfo.lineWidth = createInfo.RasterizationState.LineWidth;

    auto multisampleStateCreateInfo = VkPipelineMultisampleStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
    multisampleStateCreateInfo.rasterizationSamples = AsEnumCounterpart(createInfo.MultisampleState.SampleCount);
    multisampleStateCreateInfo.sampleShadingEnable = createInfo.MultisampleState.SampleShadingEnable;
    multisampleStateCreateInfo.minSampleShading = createInfo.MultisampleState.MinSampleShading;
    multisampleStateCreateInfo.pSampleMask = nullptr;
    multisampleStateCreateInfo.alphaToCoverageEnable = createInfo.MultisampleState.AlphaToCoverageEnable;
    multisampleStateCreateInfo.alphaToOneEnable = createInfo.MultisampleState.AlphaToOneEnable;

    auto depthStencilStateCreateInfo = VkPipelineDepthStencilStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
    depthStencilStateCreateInfo.depthTestEnable = createInfo.DepthStencilState.DepthTestEnable;
    depthStencilStateCreateInfo.depthWriteEnable = createInfo.DepthStencilState.DepthWriteEnable;
    depthStencilStateCreateInfo.depthCompareOp = AsEnumCounterpart(createInfo.DepthStencilState.DepthCompareOperator);
    depthStencilStateCreateInfo.depthBoundsTestEnable = createInfo.DepthStencilState.DepthBoundsTestEnable;
    depthStencilStateCreateInfo.stencilTestEnable = createInfo.DepthStencilState.StencilTestEnable;
    depthStencilStateCreateInfo.front = {
      .failOp = AsEnumCounterpart(createInfo.DepthStencilState.FrontStencilState.FailOperator),
      .passOp = AsEnumCounterpart(createInfo.DepthStencilState.FrontStencilState.PassOperator),
      .depthFailOp = AsEnumCounterpart(createInfo.DepthStencilState.FrontStencilState.DepthFailOperator),
      .compareOp = AsEnumCounterpart(createInfo.DepthStencilState.FrontStencilState.CompareOperator),
      .compareMask = createInfo.DepthStencilState.FrontStencilState.CompareMask,
      .writeMask = createInfo.DepthStencilState.FrontStencilState.WriteMask,
      .reference = createInfo.DepthStencilState.FrontStencilState.Reference
    };
    depthStencilStateCreateInfo.back = {
      .failOp = AsEnumCounterpart(createInfo.DepthStencilState.BackStencilState.FailOperator),
      .passOp = AsEnumCounterpart(createInfo.DepthStencilState.BackStencilState.PassOperator),
      .depthFailOp = AsEnumCounterpart(createInfo.DepthStencilState.BackStencilState.DepthFailOperator),
      .compareOp = AsEnumCounterpart(createInfo.DepthStencilState.BackStencilState.CompareOperator),
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
          auto mask = EColorComponentFlag();
          switch (type.vecsize) {
            case 4: mask |= EColorComponentFlag::E_A; RETINA_FALLTHROUGH;
            case 3: mask |= EColorComponentFlag::E_B; RETINA_FALLTHROUGH;
            case 2: mask |= EColorComponentFlag::E_G; RETINA_FALLTHROUGH;
            case 1: mask |= EColorComponentFlag::E_R; break;
            default: std::unreachable();
          }
          return mask;
        }();

        auto colorBlendAttachmentInfo = SPipelineColorBlendAttachmentInfo();
        colorBlendAttachmentInfo.BlendEnable = Core::IsFlagEnabled(colorWriteMask, EColorComponentFlag::E_A);
        colorBlendAttachmentInfo.ColorWriteMask = colorWriteMask;
        colorBlendAttachments.emplace_back(colorBlendAttachmentInfo);
      }
    }
    auto colorBlendAttachmentStates = std::vector<VkPipelineColorBlendAttachmentState>();
    colorBlendAttachmentStates.reserve(colorBlendAttachments.size());
    for (const auto& colorBlendAttachment : colorBlendAttachments) {
      colorBlendAttachmentStates.emplace_back(
        std::bit_cast<VkPipelineColorBlendAttachmentState>(colorBlendAttachment)
      );
    }

    auto colorBlendStateCreateInfo = VkPipelineColorBlendStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
    colorBlendStateCreateInfo.logicOpEnable = createInfo.ColorBlendState.LogicOperatorEnable;
    colorBlendStateCreateInfo.logicOp = AsEnumCounterpart(createInfo.ColorBlendState.LogicOperator);
    colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentStates.size();
    colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.data();
    colorBlendStateCreateInfo.blendConstants[0] = createInfo.ColorBlendState.BlendConstants[0];
    colorBlendStateCreateInfo.blendConstants[1] = createInfo.ColorBlendState.BlendConstants[1];
    colorBlendStateCreateInfo.blendConstants[2] = createInfo.ColorBlendState.BlendConstants[2];
    colorBlendStateCreateInfo.blendConstants[3] = createInfo.ColorBlendState.BlendConstants[3];

    const auto& dynamicStateInfos = createInfo.DynamicState.DynamicStates;
    auto dynamicStates = std::vector<VkDynamicState>();
    dynamicStates.reserve(dynamicStateInfos.size());
    for (const auto& dynamicStateInfo : dynamicStateInfos) {
      dynamicStates.emplace_back(AsEnumCounterpart(dynamicStateInfo));
    }

    auto dynamicStateCreateInfo = VkPipelineDynamicStateCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    auto renderingColorAttachmentFormats = std::vector<VkFormat>();
    auto renderingDepthAttachmentFormat = VkFormat();
    auto renderingStencilAttachmentFormat = VkFormat();
    auto pipelineRenderingCreateInfo = VkPipelineRenderingCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO);
    if (createInfo.RenderingInfo) {
      const auto& renderingInfo = *createInfo.RenderingInfo;
      renderingColorAttachmentFormats.reserve(renderingInfo.ColorAttachmentFormats.size());
      for (const auto& colorAttachmentFormat : renderingInfo.ColorAttachmentFormats) {
        renderingColorAttachmentFormats.emplace_back(AsEnumCounterpart(colorAttachmentFormat));
      }
      if (renderingInfo.DepthAttachmentFormat) {
        renderingDepthAttachmentFormat = AsEnumCounterpart(*renderingInfo.DepthAttachmentFormat);
      }
      if (renderingInfo.StencilAttachmentFormat) {
        renderingStencilAttachmentFormat = AsEnumCounterpart(*renderingInfo.StencilAttachmentFormat);
      }

      pipelineRenderingCreateInfo.viewMask = renderingInfo.ViewMask;
      pipelineRenderingCreateInfo.colorAttachmentCount = renderingColorAttachmentFormats.size();
      pipelineRenderingCreateInfo.pColorAttachmentFormats = renderingColorAttachmentFormats.data();
      pipelineRenderingCreateInfo.depthAttachmentFormat = renderingDepthAttachmentFormat;
      pipelineRenderingCreateInfo.stencilAttachmentFormat = renderingStencilAttachmentFormat;
    }

    const auto descriptorLayoutHandles = Details::MakeDescriptorLayoutHandles(createInfo.DescriptorLayouts);
    const auto pushConstantRange = Details::ReflectPushConstantRange(std::to_array({
      meshShaderCompiler.Get(),
      taskShaderCompiler.Get(),
      fragmentShaderCompiler.Get()
    }));
    const auto nativePushConstantInfo = std::bit_cast<VkPushConstantRange>(pushConstantRange);

    auto pipelineLayoutCreateInfo = VkPipelineLayoutCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
    pipelineLayoutCreateInfo.setLayoutCount = descriptorLayoutHandles.size();
    pipelineLayoutCreateInfo.pSetLayouts = descriptorLayoutHandles.data();
    if (pushConstantRange.Size > 0) {
      pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
      pipelineLayoutCreateInfo.pPushConstantRanges = &nativePushConstantInfo;
    }

    auto pipelineLayoutHandle = VkPipelineLayout();
    RETINA_GRAPHICS_VULKAN_CHECK(
      vkCreatePipelineLayout(
        device.GetHandle(),
        &pipelineLayoutCreateInfo,
        nullptr,
        &pipelineLayoutHandle
      )
    );

    auto pipelineCreateInfo = VkGraphicsPipelineCreateInfo(VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
    pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
    pipelineCreateInfo.stageCount = shaderStages.size();
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.pVertexInputState = nullptr;
    pipelineCreateInfo.pInputAssemblyState = nullptr;
    pipelineCreateInfo.pTessellationState = nullptr;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = pipelineLayoutHandle;
    pipelineCreateInfo.basePipelineIndex = -1;

    auto pipelineHandle = VkPipeline();
    RETINA_GRAPHICS_VULKAN_CHECK(
      vkCreateGraphicsPipelines(
        device.GetHandle(),
        VK_NULL_HANDLE,
        1,
        &pipelineCreateInfo,
        nullptr,
        &pipelineHandle
      )
    );
    RETINA_GRAPHICS_INFO("Mesh shading pipeline ({}) initialized", createInfo.Name);

    for (const auto& stage : shaderStages) {
      vkDestroyShaderModule(device.GetHandle(), stage.module, nullptr);
    }

    self->_handle = pipelineHandle;
    self->_layout = {
      .Handle = pipelineLayoutHandle,
      .PushConstant = pushConstantRange,
    };
    self->_createInfo = createInfo;
    self->_device = device.ToArcPtr();
    self->SetDebugName(createInfo.Name);
    return self;
  }

  auto CMeshShadingPipeline::GetCreateInfo() const noexcept -> const SMeshShadingPipelineCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CMeshShadingPipeline::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CMeshShadingPipeline::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_SET_DEBUG_NAME(_device->GetHandle(), _handle, VK_OBJECT_TYPE_PIPELINE, name);
    _createInfo.Name = name;
  }
}