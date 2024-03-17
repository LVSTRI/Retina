#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/CommandBufferInfo.hpp>
#include <Retina/Graphics/Enum.hpp>
#include <Retina/Graphics/Forward.hpp>

#include <filesystem>
#include <span>

namespace Retina::Graphics {
  enum class EPipelineType {
    E_GRAPHICS,
    E_MESH_SHADING,
    E_COMPUTE,
    E_RAY_TRACING,
  };

  struct SViewport {
    float32 X = 0.0f;
    float32 Y = 0.0f;
    float32 Width = 0.0f;
    float32 Height = 0.0f;
  };

  struct SScissor {
    int32 X = 0;
    int32 Y = 0;
    uint32 Width = 0;
    uint32 Height = 0;
  };

  struct SPipelineStencilState {
    EStencilOperator FailOperator = EStencilOperator::E_KEEP;
    EStencilOperator PassOperator = EStencilOperator::E_KEEP;
    EStencilOperator DepthFailOperator = EStencilOperator::E_KEEP;
    ECompareOperator CompareOperator = ECompareOperator::E_ALWAYS;
    uint32 CompareMask = 0;
    uint32 WriteMask = 0;
    uint32 Reference = 0;
  };

  struct SPipelineColorBlendAttachmentInfo {
    bool BlendEnable = false;
    EBlendFactor SourceColorBlendFactor = EBlendFactor::E_SRC_ALPHA;
    EBlendFactor DestColorBlendFactor = EBlendFactor::E_ONE_MINUS_SRC_ALPHA;
    EBlendOperator ColorBlendOperator = EBlendOperator::E_ADD;
    EBlendFactor SourceAlphaBlendFactor = EBlendFactor::E_ONE;
    EBlendFactor DestAlphaBlendFactor = EBlendFactor::E_ONE_MINUS_SRC_ALPHA;
    EBlendOperator AlphaBlendOperator = EBlendOperator::E_ADD;
    EColorComponentFlag ColorWriteMask =
      EColorComponentFlag::E_R |
      EColorComponentFlag::E_G |
      EColorComponentFlag::E_B |
      EColorComponentFlag::E_A;
  };

  struct SPipelineTessellationStateInfo {
    uint32 PatchControlPoints = 0;
  };

  struct SPipelineViewportStateInfo {
    std::vector<SViewport> Viewports = { SViewport() };
    std::vector<SScissor> Scissors = { SScissor() };
  };

  struct SPipelineRasterizationStateInfo {
    bool DepthClampEnable = false;
    bool RasterizerDiscardEnable = false;
    EPolygonMode PolygonMode = EPolygonMode::E_FILL;
    ECullModeFlag CullMode = ECullModeFlag::E_NONE;
    EFrontFace FrontFace = EFrontFace::E_COUNTER_CLOCKWISE;
    bool DepthBiasEnable = false;
    float32 DepthBiasConstantFactor = 0.0f;
    float32 DepthBiasClamp = 0.0f;
    float32 DepthBiasSlopeFactor = 0.0f;
    float32 LineWidth = 1.0f;
  };

  struct SPipelineMultisampleStateInfo {
    ESampleCountFlag SampleCount = ESampleCountFlag::E_1;
    bool SampleShadingEnable = false;
    float32 MinSampleShading = 0.0f;
    bool AlphaToCoverageEnable = false;
    bool AlphaToOneEnable = false;
  };

  struct SPipelineDepthStencilStateInfo {
    bool DepthTestEnable = false;
    bool DepthWriteEnable = false;
    ECompareOperator DepthCompareOperator = ECompareOperator::E_LESS;
    bool DepthBoundsTestEnable = false;
    bool StencilTestEnable = false;
    SPipelineStencilState FrontStencilState = {};
    SPipelineStencilState BackStencilState = {};
    float32 MinDepthBounds = 0.0f;
    float32 MaxDepthBounds = 1.0f;
  };

  struct SPipelineColorBlendStateInfo {
    bool LogicOperatorEnable = false;
    ELogicOperator LogicOperator = ELogicOperator::E_COPY;
    std::vector<SPipelineColorBlendAttachmentInfo> Attachments;
    float32 BlendConstants[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
  };

  struct SPipelineDynamicStateInfo {
    std::vector<EDynamicState> DynamicStates;
  };

  struct SPipelineRenderingInfo {
    uint32 ViewMask = 0;
    std::vector<EResourceFormat> ColorAttachmentFormats;
    std::optional<EResourceFormat> DepthAttachmentFormat = std::nullopt;
    std::optional<EResourceFormat> StencilAttachmentFormat = std::nullopt;
  };

  struct SPipelinePushConstantInfo {
    EShaderStageFlag Stages = {};
    uint32 Offset = 0;
    uint32 Size = 0;
  };

  const inline auto DEFAULT_PIPELINE_TESSELLATION_STATE_INFO = SPipelineTessellationStateInfo();
  const inline auto DEFAULT_PIPELINE_VIEWPORT_STATE_INFO = SPipelineViewportStateInfo();
  const inline auto DEFAULT_PIPELINE_RASTERIZATION_STATE_INFO = SPipelineRasterizationStateInfo();
  const inline auto DEFAULT_PIPELINE_MULTISAMPLE_STATE_INFO = SPipelineMultisampleStateInfo();
  const inline auto DEFAULT_PIPELINE_DEPTH_STENCIL_STATE_INFO = SPipelineDepthStencilStateInfo();
  const inline auto DEFAULT_PIPELINE_COLOR_BLEND_STATE_INFO = SPipelineColorBlendStateInfo();
  const inline auto DEFAULT_PIPELINE_DYNAMIC_STATE_INFO = SPipelineDynamicStateInfo();

  struct SComputePipelineCreateInfo {
    std::string Name;
    std::filesystem::path ComputeShader;
    std::vector<std::filesystem::path> IncludeDirectories;

    std::vector<Core::CReferenceWrapper<const CDescriptorLayout>> DescriptorLayouts;
  };

  struct SGraphicsPipelineCreateInfo {
    std::string Name;
    std::filesystem::path VertexShader;
    std::optional<std::filesystem::path> FragmentShader = std::nullopt;
    std::vector<std::filesystem::path> IncludeDirectories;

    std::vector<Core::CReferenceWrapper<const CDescriptorLayout>> DescriptorLayouts;

    SPipelineTessellationStateInfo TessellationState = DEFAULT_PIPELINE_TESSELLATION_STATE_INFO;
    SPipelineViewportStateInfo ViewportState = DEFAULT_PIPELINE_VIEWPORT_STATE_INFO;
    SPipelineRasterizationStateInfo RasterizationState = DEFAULT_PIPELINE_RASTERIZATION_STATE_INFO;
    SPipelineMultisampleStateInfo MultisampleState = DEFAULT_PIPELINE_MULTISAMPLE_STATE_INFO;
    SPipelineDepthStencilStateInfo DepthStencilState = DEFAULT_PIPELINE_DEPTH_STENCIL_STATE_INFO;
    SPipelineColorBlendStateInfo ColorBlendState = DEFAULT_PIPELINE_COLOR_BLEND_STATE_INFO;
    SPipelineDynamicStateInfo DynamicState = DEFAULT_PIPELINE_DYNAMIC_STATE_INFO;
    std::optional<SPipelineRenderingInfo> RenderingInfo = std::nullopt;
  };

  struct SMeshShadingPipelineCreateInfo {
    std::string Name;
    std::filesystem::path MeshShader;
    std::optional<std::filesystem::path> TaskShader = std::nullopt;
    std::optional<std::filesystem::path> FragmentShader = std::nullopt;
    std::vector<std::filesystem::path> IncludeDirectories;

    std::vector<Core::CReferenceWrapper<const CDescriptorLayout>> DescriptorLayouts;

    SPipelineViewportStateInfo ViewportState = DEFAULT_PIPELINE_VIEWPORT_STATE_INFO;
    SPipelineRasterizationStateInfo RasterizationState = DEFAULT_PIPELINE_RASTERIZATION_STATE_INFO;
    SPipelineMultisampleStateInfo MultisampleState = DEFAULT_PIPELINE_MULTISAMPLE_STATE_INFO;
    SPipelineDepthStencilStateInfo DepthStencilState = DEFAULT_PIPELINE_DEPTH_STENCIL_STATE_INFO;
    SPipelineColorBlendStateInfo ColorBlendState = DEFAULT_PIPELINE_COLOR_BLEND_STATE_INFO;
    SPipelineDynamicStateInfo DynamicState = DEFAULT_PIPELINE_DYNAMIC_STATE_INFO;
    std::optional<SPipelineRenderingInfo> RenderingInfo = std::nullopt;
  };
}
