#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Pipeline.hpp>
#include <Retina/Graphics/PipelineInfo.hpp>

namespace Retina {
    class CGraphicsPipeline : public IPipeline {
    public:
        using Self = CGraphicsPipeline;

        CGraphicsPipeline() noexcept;
        virtual ~CGraphicsPipeline() noexcept override = default;

        RETINA_NODISCARD static auto Make(
            const CDevice& device,
            const SGraphicsPipelineCreateInfo& createInfo
        ) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SGraphicsPipelineCreateInfo&;

    private:
        SGraphicsPipelineCreateInfo _createInfo = {};
    };
}