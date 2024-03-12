#pragma once

#include <Retina/Graphics/PipelineInfo.hpp>

#include <vulkan/vulkan.h>

#include <filesystem>
#include <span>
#include <vector>

namespace spirv_cross {
  class CompilerGLSL;
}

namespace Retina::Graphics {
  namespace Details {
    RETINA_NODISCARD auto CompileShaderFromSource(
      const std::filesystem::path& path,
      std::span<const std::filesystem::path> includeDirectories,
      EShaderStageFlag stage
    ) noexcept -> std::vector<uint32>;

    RETINA_NODISCARD auto MakeShaderModule(const CDevice& device, std::span<const uint32> spirv) noexcept -> VkShaderModule;

    RETINA_NODISCARD auto ExecutionModelToShaderStage(const spirv_cross::CompilerGLSL& compiler) noexcept -> EShaderStageFlag;

    RETINA_NODISCARD auto ReflectPushConstantRange(
      std::span<const spirv_cross::CompilerGLSL* const> compilers
    ) noexcept -> SPipelinePushConstantInfo;

    RETINA_NODISCARD auto MakeDescriptorLayoutHandles(
      std::span<const Core::CReferenceWrapper<const CDescriptorLayout>> layouts
    ) noexcept -> std::vector<VkDescriptorSetLayout>;
  }

  class IPipeline : public Core::IEnableIntrusiveReferenceCount<IPipeline> {
  private:
    struct SPipelineLayout {
      VkPipelineLayout Handle = {};
      SPipelinePushConstantInfo PushConstant = {};
    };

  public:
    virtual ~IPipeline() noexcept;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkPipeline;
    RETINA_NODISCARD auto GetLayout() const noexcept -> const SPipelineLayout&;
    RETINA_NODISCARD auto GetType() const noexcept -> EPipelineType;
    RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

    RETINA_NODISCARD auto GetLayoutHandle() const noexcept -> VkPipelineLayout;
    RETINA_NODISCARD auto GetLayoutPushConstantInfo() const noexcept -> const SPipelinePushConstantInfo&;

    RETINA_NODISCARD auto GetBindPoint() const noexcept -> EPipelineBindPoint;

  protected:
    IPipeline(EPipelineType type) noexcept;

  protected:
    VkPipeline _handle = {};
    SPipelineLayout _layout = {};
    EPipelineType _type;

    Core::CArcPtr<const CDevice> _device;
  };
}
