#include <Retina/Graphics/PipelineInfo.hpp>

#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/DescriptorLayout.hpp>

#include <volk.h>

namespace Retina {
    auto MakeShaderModule(const CDevice& device, std::span<const uint32> spirv) noexcept -> VkShaderModule {
        RETINA_PROFILE_SCOPED();
        auto shaderModuleCreateInfo = VkShaderModuleCreateInfo(VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
        shaderModuleCreateInfo.codeSize = spirv.size_bytes();
        shaderModuleCreateInfo.pCode = spirv.data();
        auto shaderModuleHandle = VkShaderModule();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkCreateShaderModule(
                device.GetHandle(),
                &shaderModuleCreateInfo,
                nullptr,
                &shaderModuleHandle
            )
        );
        return shaderModuleHandle;
    }

    auto ReflectPushConstantRange(const spirv_cross::CompilerHLSL* compiler) noexcept -> VkPushConstantRange {
        RETINA_PROFILE_SCOPED();
        const auto resources = compiler->get_shader_resources();
        auto pushConstantRange = VkPushConstantRange();
        if (!resources.push_constant_buffers.empty()) {
            const auto& pushConstantBuffer = resources.push_constant_buffers[0];
            const auto& pushConstantType = compiler->get_type(pushConstantBuffer.base_type_id);
            pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL;
            pushConstantRange.offset = 0;
            pushConstantRange.size = compiler->get_declared_struct_size(pushConstantType);
        }
        return pushConstantRange;
    }

    auto MakeDescriptorLayoutHandles(
        std::span<const std::reference_wrapper<const CDescriptorLayout>> descriptorLayouts
    ) noexcept -> std::vector<VkDescriptorSetLayout> {
        RETINA_PROFILE_SCOPED();
        auto descriptorLayoutHandles = std::vector<VkDescriptorSetLayout>();
        descriptorLayoutHandles.reserve(descriptorLayouts.size());
        for (const auto& descriptorLayout : descriptorLayouts) {
            descriptorLayoutHandles.push_back(descriptorLayout.get().GetHandle());
        }
        return descriptorLayoutHandles;
    }
}