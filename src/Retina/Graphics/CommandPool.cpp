#include <Retina/Graphics/CommandPool.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Queue.hpp>

#include <volk.h>

namespace Retina {
    CCommandPool::~CCommandPool() noexcept {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(_queue->GetDevice().GetLogger(), "Destroying Command Pool: \"{}\"", GetDebugName());
        vkDestroyCommandPool(_queue->GetDevice().GetHandle(), _handle, nullptr);
    }

    auto CCommandPool::Make(const CQueue& queue, const SCommandPoolCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto commandPool = CArcPtr(new Self());
        RETINA_LOG_INFO(queue.GetLogger(), "Creating Command Pool: \"{}\"", createInfo.Name);
        const auto familyIndex = queue.GetFamilyIndex();
        auto commandPoolCreateInfo = VkCommandPoolCreateInfo(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
        commandPoolCreateInfo.flags = ToEnumCounterpart(createInfo.Flags);
        commandPoolCreateInfo.queueFamilyIndex = familyIndex;

        auto commandPoolHandle = VkCommandPool();
        RETINA_VULKAN_CHECK(
            queue.GetLogger(),
            vkCreateCommandPool(
                queue.GetDevice().GetHandle(),
                &commandPoolCreateInfo,
                nullptr,
                &commandPoolHandle
            )
        );
        commandPool->_handle = commandPoolHandle;
        commandPool->_createInfo = createInfo;
        commandPool->_queue = queue.ToArcPtr();
        commandPool->SetDebugName(createInfo.Name);
        return commandPool;
    }

    auto CCommandPool::Make(
        const CQueue& queue,
        uint32 count,
        const SCommandPoolCreateInfo& createInfo
    ) noexcept -> std::vector<CArcPtr<Self>> {
        RETINA_PROFILE_SCOPED();
        auto commandPools = std::vector<CArcPtr<Self>>();
        commandPools.reserve(count);
        for (auto i = 0_u32; i < count; ++i) {
            auto newCreateInfo = createInfo;
            newCreateInfo.Name += std::to_string(i);
            commandPools.push_back(Make(queue, newCreateInfo));
        }
        return commandPools;
    }

    auto CCommandPool::GetHandle() const noexcept -> VkCommandPool {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto CCommandPool::GetCreateInfo() const noexcept -> const SCommandPoolCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CCommandPool::GetQueue() const noexcept -> const CQueue& {
        RETINA_PROFILE_SCOPED();
        return *_queue;
    }

    auto CCommandPool::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_COMMAND_POOL;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(
            _queue->GetDevice().GetLogger(),
            vkSetDebugUtilsObjectNameEXT(
                _queue->GetDevice().GetHandle(),
                &info
            )
        );
    }

    auto CCommandPool::Reset() const noexcept -> void {
        RETINA_PROFILE_SCOPED();
        RETINA_VULKAN_CHECK(_queue->GetDevice().GetLogger(), vkResetCommandPool(_queue->GetDevice().GetHandle(), _handle, 0));
    }
}
