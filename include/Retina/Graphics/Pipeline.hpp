#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/PipelineInfo.hpp>

#include <vulkan/vulkan.h>

#include <filesystem>
#include <span>

namespace Retina {
    class IPipeline : public INativeDebugName, public IEnableIntrusiveReferenceCount<IPipeline> {
    public:
        using Self = IPipeline;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkPipeline;
        RETINA_NODISCARD auto GetLayout() const noexcept -> const SPipelineLayout&;
        RETINA_NODISCARD auto GetType() const noexcept -> EPipelineType;
        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

        auto SetDebugName(std::string_view name) noexcept -> void;

        RETINA_NODISCARD auto GetLayoutPushConstantInfo() const noexcept -> const SPipelinePushConstantInfo&;
        RETINA_NODISCARD auto GetLayoutHandle() const noexcept -> VkPipelineLayout;

    protected:
        IPipeline(EPipelineType type) noexcept;
        virtual ~IPipeline() noexcept;

        VkPipeline _handle = {};
        SPipelineLayout _layout = {};
        EPipelineType _type = {};

        CArcPtr<const CDevice> _device;
    };

    RETINA_NODISCARD auto CompileShaderFromSource(
        const CDevice& device,
        const fs::path& path,
        std::span<const fs::path> includeDirectories,
        EShaderStage shaderStage
    ) noexcept -> std::vector<uint32>;

    RETINA_NODISCARD auto MakeShaderModule(const CDevice& device, std::span<const uint32> spirv) noexcept -> VkShaderModule;

    RETINA_NODISCARD auto ExecutionModelToShaderStage(const spirv_cross::CompilerGLSL& compiler) noexcept -> EShaderStage;

    RETINA_NODISCARD auto ReflectPushConstantRange(
        std::span<const spirv_cross::CompilerGLSL* const> compilers
    ) noexcept -> SPipelinePushConstantInfo;

    RETINA_NODISCARD auto MakeDescriptorLayoutHandles(
        std::span<const std::reference_wrapper<const CDescriptorLayout>> descriptorLayouts
    ) noexcept -> std::vector<VkDescriptorSetLayout>;
}
