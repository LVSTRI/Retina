#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/QueryPool.hpp>

#include <volk.h>

namespace Retina {
    CQueryPool::~CQueryPool() noexcept {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(_device->GetLogger(), "Destroying QueryPool: \"{}\"", GetDebugName());
        vkDestroyQueryPool(_device->GetHandle(), _handle, nullptr);
    }

    auto CQueryPool::Make(const CDevice& device, const SQueryPoolCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto queryPool = CArcPtr(new Self());
        RETINA_LOG_INFO(device.GetLogger(), "Creating QueryPool: \"{}\"", createInfo.Name);

        auto queryPoolCreateInfo = VkQueryPoolCreateInfo(VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO);
        queryPoolCreateInfo.queryType = ToEnumCounterpart(createInfo.Type);
        queryPoolCreateInfo.queryCount = createInfo.Count;
        queryPoolCreateInfo.pipelineStatistics = ToEnumCounterpart(createInfo.PipelineStatisticFlags);

        auto queryPoolHandle = VkQueryPool();
        RETINA_VULKAN_CHECK(device.GetLogger(), vkCreateQueryPool(device.GetHandle(), &queryPoolCreateInfo, nullptr, &queryPoolHandle));

        queryPool->_handle = queryPoolHandle;
        queryPool->_createInfo = createInfo;
        queryPool->_device = device.ToArcPtr();
        queryPool->SetDebugName(createInfo.Name);

        return queryPool;
    }

    auto CQueryPool::GetHandle() const noexcept -> VkQueryPool {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto CQueryPool::GetType() const noexcept -> EQueryType {
        RETINA_PROFILE_SCOPED();
        return _createInfo.Type;
    }

    auto CQueryPool::GetCount() const noexcept -> uint32 {
        RETINA_PROFILE_SCOPED();
        return _createInfo.Count;
    }

    auto CQueryPool::GetCreateInfo() const noexcept -> const SQueryPoolCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CQueryPool::GetDevice() const noexcept -> const CDevice& {
        RETINA_PROFILE_SCOPED();
        return *_device;
    }

    auto CQueryPool::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_QUERY_POOL;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(_device->GetLogger(), vkSetDebugUtilsObjectNameEXT(_device->GetHandle(), &info));
    }

    auto CQueryPool::FetchResult(EQueryResultFlag flags) const noexcept -> std::optional<std::vector<uint8>> {
        RETINA_PROFILE_SCOPED();
        const auto stride = IsFlagEnabled(flags, EQueryResultFlag::E_64_BIT)
            ? sizeof(uint64)
            : sizeof(uint32);
        const auto isAvailabilityRequested = IsFlagEnabled(flags, EQueryResultFlag::E_WITH_AVAILABILITY);
        auto data = std::vector<uint8>();
        if (isAvailabilityRequested) {
            data.resize(_createInfo.Count * stride * 2);
        } else {
            data.resize(_createInfo.Count * stride);
        }
        const auto result = vkGetQueryPoolResults(
            _device->GetHandle(),
            _handle,
            0,
            _createInfo.Count,
            data.size(),
            data.data(),
            stride,
            ToEnumCounterpart(flags)
        );
        if (result == VK_NOT_READY) {
            return std::nullopt;
        }
        RETINA_VULKAN_CHECK(_device->GetLogger(), result);
        return data;
    }
}