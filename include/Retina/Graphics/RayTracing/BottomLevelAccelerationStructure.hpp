#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/RayTracing/AccelerationStructure.hpp>
#include <Retina/Graphics/RayTracing/AccelerationStructureInfo.hpp>

#include <span>

namespace Retina {
    class CBottomLevelAccelerationStructure : public IAccelerationStructure {
    public:
        using Self = CBottomLevelAccelerationStructure;

        CBottomLevelAccelerationStructure() noexcept;
        ~CBottomLevelAccelerationStructure() noexcept override = default;

        RETINA_NODISCARD static auto Make(
            const CDevice& device,
            const SAccelerationStructureCreateInfo& createInfo
        ) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD static auto MakeCompact(
            const CDevice& device,
            const SAccelerationStructureCreateInfo& createInfo
        ) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SAccelerationStructureCreateInfo&;

    private:
        SAccelerationStructureCreateInfo _createInfo = {};
    };
}
