#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/DescriptorSetInfo.hpp>

#include <vulkan/vulkan.h>

#include <span>

namespace Retina {
    class CDescriptorSet : public INativeDebugName, public IEnableIntrusiveReferenceCount<CDescriptorSet> {
    public:
        using Self = CDescriptorSet;

        CDescriptorSet() noexcept = default;
        ~CDescriptorSet() noexcept;

        RETINA_NODISCARD static auto Make(const CDevice& device, const SDescriptorSetCreateInfo& createInfo) noexcept -> CArcPtr<Self>;
        RETINA_NODISCARD static auto Make(
            const CDevice& device,
            uint32 count,
            const SDescriptorSetCreateInfo& createInfo
        ) noexcept -> std::vector<CArcPtr<Self>>;
        RETINA_NODISCARD static auto Make(
            const CDevice& device,
            std::span<const SDescriptorSetCreateInfo> createInfos
        ) noexcept -> std::vector<CArcPtr<Self>>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkDescriptorSet;
        RETINA_NODISCARD auto GetLayout() const noexcept -> const CDescriptorLayout&;
        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SDescriptorSetCreateInfo&;

        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;
        RETINA_NODISCARD auto GetDescriptorPool() const noexcept -> const CDescriptorPool&;

        auto SetDebugName(std::string_view name) noexcept -> void;

        auto Write(std::span<const SDescriptorWriteInfo> writes) const noexcept -> void;

    private:
        VkDescriptorSet _handle = {};

        SDescriptorSetCreateInfo _createInfo = {};
    };
}
