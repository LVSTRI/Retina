#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/RayTracing/AccelerationStructure.hpp>
#include <Retina/Graphics/RayTracing/AccelerationStructureInfo.hpp>

namespace Retina {
    class CTopLevelAccelerationStructure : public IAccelerationStructure {
    public:
        using Self = CTopLevelAccelerationStructure;

        CTopLevelAccelerationStructure() noexcept;
        ~CTopLevelAccelerationStructure() noexcept override = default;

        RETINA_NODISCARD static auto Make(
            const CDevice& device,
            const SAccelerationStructureCreateInfo& createInfo
        ) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SAccelerationStructureCreateInfo&;

    private:
        SAccelerationStructureCreateInfo _createInfo = {};
    };
}
