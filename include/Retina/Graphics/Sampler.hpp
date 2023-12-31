#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/SamplerInfo.hpp>

namespace Retina {
    class CSampler : public INativeDebugName, public IEnableIntrusiveReferenceCount<CSampler> {
    public:
        using Self = CSampler;

        CSampler() noexcept = default;
        ~CSampler() noexcept;

        RETINA_NODISCARD static auto Make(const CDevice& device, const SSamplerCreateInfo& createInfo) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkSampler;
        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

        auto SetDebugName(std::string_view name) noexcept -> void;

        RETINA_NODISCARD auto GetDescriptor() const noexcept -> SImageDescriptor;

    private:
        VkSampler _handle = {};

        CArcPtr<const CDevice> _device;
    };
}
