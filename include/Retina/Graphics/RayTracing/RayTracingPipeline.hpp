#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Buffer.hpp>
#include <Retina/Graphics/Pipeline.hpp>
#include <Retina/Graphics/PipelineInfo.hpp>

namespace Retina {
    struct SShaderBindingTable {
        CArcPtr<CBuffer> Buffer;
        std::vector<SShaderBindingTableRegion> Regions;
    };

    class CRayTracingPipeline : public IPipeline {
    public:
        using Self = CRayTracingPipeline;

        CRayTracingPipeline() noexcept;
        ~CRayTracingPipeline() noexcept override = default;

        RETINA_NODISCARD static auto Make(
            const CDevice& device,
            const SRayTracingPipelineCreateInfo& createInfo
        ) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetShaderBindingTable() const noexcept -> const SShaderBindingTable&;
        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SRayTracingPipelineCreateInfo&;

    private:
        SShaderBindingTable _shaderBindingTable = {};
        SRayTracingPipelineCreateInfo _createInfo = {};
    };
}