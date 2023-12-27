#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/InstanceInfo.hpp>

#include <spdlog/spdlog.h>

#include <vulkan/vulkan.h>

#include <memory>

namespace Retina {
    class CInstance : public IEnableIntrusiveReferenceCount<CInstance> {
    public:
        using Self = CInstance;

        CInstance() noexcept = default;
        ~CInstance() noexcept;

        RETINA_NODISCARD static auto Make(const SInstanceCreateInfo& createInfo) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkInstance;
        RETINA_NODISCARD auto GetVersion() const noexcept -> uint32;
        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SInstanceCreateInfo&;
        RETINA_NODISCARD auto GetLogger() const noexcept -> spdlog::logger&;

        RETINA_NODISCARD auto IsFeatureEnabled(bool SInstanceFeatureInfo::* feature) const noexcept -> bool;

    private:
        VkInstance _handle = {};
        uint32 _version = 0;

        SInstanceCreateInfo _createInfo = {};
        std::shared_ptr<spdlog::logger> _logger;
    };
}
