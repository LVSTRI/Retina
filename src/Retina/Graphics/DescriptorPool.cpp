#include <Retina/Graphics/DescriptorPool.hpp>
#include <Retina/Graphics/Device.hpp>

#include <volk.h>

namespace Retina {
    CDescriptorPool::~CDescriptorPool() noexcept {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(_device->GetLogger(), "Destroying Descriptor Pool: \"{}\"", GetDebugName());
        vkDestroyDescriptorPool(_device->GetHandle(), _handle, nullptr);
    }

    auto CDescriptorPool::Make(const CDevice& device, const SDescriptorPoolCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto descriptorPool = CArcPtr(new Self());
        RETINA_LOG_INFO(device.GetLogger(), "Creating Descriptor Pool: \"{}\"", createInfo.Name);
        const auto maxSets = [&] {
            auto maxSets = 0_u32;
            for (const auto& [_, size] : createInfo.Sizes) {
                maxSets += size;
            }
            return maxSets;
        }();
        auto descriptorPoolSizes = std::vector<VkDescriptorPoolSize>();
        descriptorPoolSizes.reserve(createInfo.Sizes.size());
        for (const auto& [type, size] : createInfo.Sizes) {
            descriptorPoolSizes.push_back({
                .type = ToEnumCounterpart(type),
                .descriptorCount = size
            });
        }
        auto descriptorPoolCreateInfo = VkDescriptorPoolCreateInfo(VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
        descriptorPoolCreateInfo.flags = ToEnumCounterpart(createInfo.Flags);
        descriptorPoolCreateInfo.maxSets = maxSets;
        descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32>(descriptorPoolSizes.size());
        descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();

        auto descriptorPoolHandle = VkDescriptorPool();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkCreateDescriptorPool(
                device.GetHandle(),
                &descriptorPoolCreateInfo,
                nullptr,
                &descriptorPoolHandle
            )
        );
        descriptorPool->_handle = descriptorPoolHandle;
        descriptorPool->_createInfo = createInfo;
        descriptorPool->_device = device.ToArcPtr();
        descriptorPool->SetDebugName(createInfo.Name);

        return descriptorPool;
    }

    auto CDescriptorPool::Make(
        const CDevice& device,
        uint32 count,
        const SDescriptorPoolCreateInfo& createInfo
    ) noexcept -> std::vector<CArcPtr<Self>> {
        RETINA_PROFILE_SCOPED();
        auto descriptorPools = std::vector<CArcPtr<Self>>();
        descriptorPools.reserve(count);
        for (auto i = 0_u32; i < count; ++i) {
            auto newCreateInfo = createInfo;
            newCreateInfo.Name += std::to_string(i);
            descriptorPools.push_back(Make(device, newCreateInfo));
        }
        return descriptorPools;
    }

    auto CDescriptorPool::GetHandle() const noexcept -> VkDescriptorPool {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto CDescriptorPool::GetCreateInfo() const noexcept -> const SDescriptorPoolCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CDescriptorPool::GetDevice() const noexcept -> const CDevice& {
        RETINA_PROFILE_SCOPED();
        return *_device;
    }

    auto CDescriptorPool::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(_device->GetLogger(), vkSetDebugUtilsObjectNameEXT(_device->GetHandle(), &info));
    }

    auto CDescriptorPool::Reset() const noexcept -> void {
        RETINA_PROFILE_SCOPED();
        RETINA_VULKAN_CHECK(_device->GetLogger(), vkResetDescriptorPool(_device->GetHandle(), _handle, {}));
    }
}
