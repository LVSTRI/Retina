#pragma once

#include <Retina/Graphics/Pipeline.hpp>

namespace Retina::Graphics {
  class CGraphicsPipeline : public IPipeline {
  public:
    CGraphicsPipeline() noexcept;
    ~CGraphicsPipeline() noexcept override;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      const SGraphicsPipelineCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CGraphicsPipeline>;

    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SGraphicsPipelineCreateInfo&;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

  private:
    SGraphicsPipelineCreateInfo _createInfo = {};
  };
}
