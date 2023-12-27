#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/DeviceInfo.hpp>
#include <Retina/Graphics/Native/NativeDebugName.hpp>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <spdlog/spdlog.h>

namespace Retina {
    class CDevice : public INativeDebugName, public IEnableIntrusiveReferenceCount<CDevice>  {
    public:
        using Self = CDevice;

        CDevice() noexcept = default;
        ~CDevice() noexcept;

        RETINA_NODISCARD static auto Make(const CInstance& instance, const SDeviceCreateInfo& createInfo) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkDevice;
        RETINA_NODISCARD auto GetPhysicalDevice() const noexcept -> VkPhysicalDevice;
        RETINA_NODISCARD auto GetAllocator() const noexcept -> VmaAllocator;

        RETINA_NODISCARD auto GetGraphicsQueue() const noexcept -> CQueue&;
        RETINA_NODISCARD auto GetComputeQueue() const noexcept -> CQueue&;
        RETINA_NODISCARD auto GetTransferQueue() const noexcept -> CQueue&;

        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SDeviceCreateInfo&;
        RETINA_NODISCARD auto GetLogger() const noexcept -> spdlog::logger&;
        RETINA_NODISCARD auto GetInstance() const noexcept -> const CInstance&;

        auto SetDebugName(std::string_view name) noexcept -> void;

        auto WaitIdle() const noexcept -> void;

    private:
        VkDevice _handle = {};
        VkPhysicalDevice _physicalDevice = {};
        VmaAllocator _allocator = {};
        SPhysicalDeviceProperties _physicalDeviceProperties = {};

        CArcPtr<CQueue> _graphicsQueue;
        CArcPtr<CQueue> _computeQueue;
        CArcPtr<CQueue> _transferQueue;

        SDeviceCreateInfo _createInfo = {};
        std::shared_ptr<spdlog::logger> _logger;
        CArcPtr<const CInstance> _instance;
    };
}
