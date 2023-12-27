#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/CommandPoolInfo.hpp>

#include <vulkan/vulkan.h>

namespace Retina {
    class CCommandPool : public INativeDebugName, public IEnableIntrusiveReferenceCount<CCommandPool> {
    public:
        using Self = CCommandPool;

        CCommandPool() noexcept = default;
        ~CCommandPool() noexcept;

        RETINA_NODISCARD static auto Make(const CQueue& queue, const SCommandPoolCreateInfo& createInfo) noexcept -> CArcPtr<Self>;
        RETINA_NODISCARD static auto Make(
            const CQueue& queue,
            uint32 count,
            const SCommandPoolCreateInfo& createInfo
        ) noexcept -> std::vector<CArcPtr<Self>>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkCommandPool;
        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SCommandPoolCreateInfo&;
        RETINA_NODISCARD auto GetQueue() const noexcept -> const CQueue&;

        auto SetDebugName(std::string_view name) noexcept -> void;

        auto Reset() const noexcept -> void;

    private:
        VkCommandPool _handle = {};

        SCommandPoolCreateInfo _createInfo = {};
        CArcPtr<const CQueue> _queue;
    };
}
