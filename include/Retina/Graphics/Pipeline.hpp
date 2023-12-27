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
        RETINA_NODISCARD auto GetLayout() const noexcept -> VkPipelineLayout;
        RETINA_NODISCARD auto GetType() const noexcept -> EPipelineType;

        auto SetDebugName(std::string_view name) noexcept -> void;

    protected:
        IPipeline(EPipelineType type) noexcept;
        virtual ~IPipeline() noexcept;

        VkPipeline _handle = {};
        VkPipelineLayout _layout = {};
        EPipelineType _type = {};

        CArcPtr<const CDevice> _device;
    };

    RETINA_NODISCARD auto CompileShaderFromSource(
        const CDevice& device,
        const fs::path& path,
        std::span<const fs::path> includeDirectories,
        std::wstring_view entryPoint,
        EShaderStage shaderStage
    ) noexcept -> std::vector<uint32>;
}
