#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Pipeline.hpp>
#include <Retina/Graphics/PipelineInfo.hpp>

namespace Retina {
    class CComputePipeline : public IPipeline {
    public:
        using Self = CComputePipeline;

        CComputePipeline() noexcept;
        ~CComputePipeline() noexcept override = default;

        RETINA_NODISCARD static auto Make(
            const CDevice& device,
            const SComputePipelineCreateInfo& createInfo
        ) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SComputePipelineCreateInfo&;

    private:
        SComputePipelineCreateInfo _createInfo = {};
    };
}