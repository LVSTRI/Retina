#pragma once

#include <Retina/Core/Core.hpp>

namespace Retina {
    struct SSamplerFilterInfo {
        ESamplerFilter Min = ESamplerFilter::E_NEAREST;
        ESamplerFilter Mag = Min;
    };

    struct SSamplerAddressInfo {
        ESamplerAddressMode U = ESamplerAddressMode::E_REPEAT;
        ESamplerAddressMode V = U;
        ESamplerAddressMode W = U;
    };

    struct SSamplerCreateInfo {
        std::string Name;
        SSamplerFilterInfo Filter;
        SSamplerAddressInfo Address;
        ESamplerMipmapMode MipmapMode = ESamplerMipmapMode::E_NEAREST;
        ESamplerBorderColor BorderColor = ESamplerBorderColor::E_FLOAT_TRANSPARENT_BLACK;
        bool CompareEnable = false;
        ECompareOperator CompareOperator = ECompareOperator::E_NEVER;
        bool AnisotropyEnable = false;
        float32 Anisotropy = 0.0f;
        float32 LodBias = 0.0f;

        std::optional<ESamplerReductionMode> ReductionMode = std::nullopt;
    };
}
