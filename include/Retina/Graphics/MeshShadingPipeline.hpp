#pragma once

#include <Retina/Graphics/Pipeline.hpp>

namespace Retina::Graphics {
  class CMeshShadingPipeline : public IPipeline {
  public:
    CMeshShadingPipeline() noexcept;
    ~CMeshShadingPipeline() noexcept override;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      const SMeshShadingPipelineCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CMeshShadingPipeline>;

    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SMeshShadingPipelineCreateInfo&;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

  private:
    SMeshShadingPipelineCreateInfo _createInfo = {};
  };
}
