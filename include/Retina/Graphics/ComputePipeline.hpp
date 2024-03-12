#pragma once

#include <Retina/Graphics/Pipeline.hpp>

namespace Retina::Graphics {
  class CComputePipeline : public IPipeline {
  public:
    CComputePipeline() noexcept;
    ~CComputePipeline() noexcept override;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      const SComputePipelineCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CComputePipeline>;

    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SComputePipelineCreateInfo&;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

  private:
    SComputePipelineCreateInfo _createInfo = {};
  };
}
