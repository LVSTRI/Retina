#include <Retina/Graphics/ComputePipeline.hpp>
#include <Retina/Graphics/DescriptorLayout.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>

#include <spirv_glsl.hpp>

#include <volk.h>

#include <vector>
#include <array>


namespace Retina::Graphics {
  CComputePipeline::CComputePipeline() noexcept
    : IPipeline(EPipelineType::E_COMPUTE)
  {
    RETINA_PROFILE_SCOPED();
  }

  CComputePipeline::~CComputePipeline() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      RETINA_GRAPHICS_INFO("Compute pipeline ({}) destroyed", GetDebugName());
    }
  }

  auto CComputePipeline::Make(
    const CDevice& device,
    const SComputePipelineCreateInfo& createInfo
  ) noexcept -> Core::CArcPtr<CComputePipeline> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CComputePipeline());

    const auto computeShaderBinary = Details::CompileShaderFromSource(
      createInfo.ComputeShader,
      createInfo.IncludeDirectories,
      EShaderStageFlag::E_COMPUTE
    );
    const auto computeShaderCompiler = std::make_shared<spirv_cross::CompilerGLSL>(computeShaderBinary);
    auto computeShaderStage = VkPipelineShaderStageCreateInfo(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
    computeShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStage.module = Details::MakeShaderModule(device, computeShaderBinary);
    computeShaderStage.pName = "main";

    const auto descriptorLayoutHandles = Details::MakeDescriptorLayoutHandles(createInfo.DescriptorLayouts);
    const auto pushConstantRange = Details::ReflectPushConstantRange(std::to_array({
      computeShaderCompiler.get(),
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

    auto pipelineCreateInfo = VkComputePipelineCreateInfo(VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO);
    pipelineCreateInfo.stage = computeShaderStage;
    pipelineCreateInfo.layout = pipelineLayoutHandle;
    pipelineCreateInfo.basePipelineIndex = -1;

    auto pipelineHandle = VkPipeline();
    RETINA_GRAPHICS_VULKAN_CHECK(
      vkCreateComputePipelines(
        device.GetHandle(),
        VK_NULL_HANDLE,
        1,
        &pipelineCreateInfo,
        nullptr,
        &pipelineHandle
      )
    );
    RETINA_GRAPHICS_INFO("Compute pipeline ({}) initialized", createInfo.Name);

    self->_handle = pipelineHandle;
    self->_layout = {
      .Handle = pipelineLayoutHandle,
      .PushConstant = pushConstantRange,
    };
    self->_createInfo = createInfo;
    self->_device = device.ToArcPtr();
    return self;
  }

  auto CComputePipeline::GetCreateInfo() const noexcept -> const SComputePipelineCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CComputePipeline::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CComputePipeline::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_SET_DEBUG_NAME(_device->GetHandle(), _handle, VK_OBJECT_TYPE_PIPELINE, name);
    _createInfo.Name = name;
  }
}
