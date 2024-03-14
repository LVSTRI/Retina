#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Enum.hpp>
#include <Retina/Graphics/Forward.hpp>

#include <string>

namespace Retina::Graphics {
  struct SSamplerFilterInfo {
    EFilter Min = EFilter::E_NEAREST;
    EFilter Mag = Min;
  };

  struct SSamplerAddressModeInfo {
    ESamplerAddressMode U = ESamplerAddressMode::E_REPEAT;
    ESamplerAddressMode V = U;
    ESamplerAddressMode W = U;
  };

  struct SSamplerCreateInfo {
    std::string Name;
    SSamplerFilterInfo Filter = {};
    SSamplerAddressModeInfo Address = {};
    ESamplerMipmapMode MipmapMode = ESamplerMipmapMode::E_NEAREST;
    EBorderColor BorderColor = EBorderColor::E_FLOAT_OPAQUE_BLACK;
    bool CompareEnable = false;
    ECompareOperator CompareOperator = ECompareOperator::E_NEVER;
    bool AnisotropyEnable = false;
    float32 Anisotropy = 0.0f;
    float32 LodBias = 0.0f;

    std::optional<ESamplerReductionMode> ReductionMode = std::nullopt;
  };
}
