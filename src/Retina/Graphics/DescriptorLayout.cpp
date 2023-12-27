#include <Retina/Graphics/DescriptorLayout.hpp>
#include <Retina/Graphics/DescriptorPool.hpp>
#include <Retina/Graphics/Device.hpp>

#include <volk.h>

namespace Retina {
    CDescriptorLayout::~CDescriptorLayout() noexcept {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(GetDevice().GetLogger(), "Destroying Descriptor Layout: \"{}\"", GetDebugName());
        vkDestroyDescriptorSetLayout(GetDevice().GetHandle(), _handle, nullptr);
    }

    auto CDescriptorLayout::Make(const CDescriptorPool& descriptorPool, const SDescriptorLayoutCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto descriptorLayout = CArcPtr(new Self());
        const auto& device = descriptorPool.GetDevice();
        RETINA_LOG_INFO(device.GetLogger(), "Creating Descriptor Layout: \"{}\"", createInfo.Name);
        auto descriptorLayoutBindings = std::vector<VkDescriptorSetLayoutBinding>();
        descriptorLayoutBindings.reserve(createInfo.Bindings.size());
        auto descriptorLayoutBindingFlags = std::vector<VkDescriptorBindingFlags>();
        descriptorLayoutBindingFlags.reserve(createInfo.Bindings.size());
        for (auto binding = 0_u32; const auto& each : createInfo.Bindings) {
            descriptorLayoutBindings.push_back({
                .binding = binding++,
                .descriptorType = ToEnumCounterpart(each.Type),
                .descriptorCount = each.Count,
                .stageFlags = static_cast<VkShaderStageFlags>(ToEnumCounterpart(each.Stage)),
                .pImmutableSamplers = nullptr
            });
            descriptorLayoutBindingFlags.push_back(ToEnumCounterpart(each.Flags));
        }

        const auto& descriptorPoolInfo = descriptorPool.GetCreateInfo();
        auto descriptorBindingFlagInfo = VkDescriptorSetLayoutBindingFlagsCreateInfo(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT);
        descriptorBindingFlagInfo.bindingCount = static_cast<uint32>(descriptorLayoutBindingFlags.size());
        descriptorBindingFlagInfo.pBindingFlags = descriptorLayoutBindingFlags.data();

        auto descriptorLayoutCreateInfo = VkDescriptorSetLayoutCreateInfo(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
        descriptorLayoutCreateInfo.pNext = &descriptorBindingFlagInfo;
        descriptorLayoutCreateInfo.flags = ToEnumCounterpart(createInfo.Flags);
        if ((descriptorPoolInfo.Flags & EDescriptorPoolCreateFlag::E_UPDATE_AFTER_BIND_BIT) == EDescriptorPoolCreateFlag::E_UPDATE_AFTER_BIND_BIT) {
            descriptorLayoutCreateInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        }
        descriptorLayoutCreateInfo.bindingCount = static_cast<uint32>(descriptorLayoutBindings.size());
        descriptorLayoutCreateInfo.pBindings = descriptorLayoutBindings.data();

        auto descriptorLayoutHandle = VkDescriptorSetLayout();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkCreateDescriptorSetLayout(
                device.GetHandle(),
                &descriptorLayoutCreateInfo,
                nullptr,
                &descriptorLayoutHandle
            )
        );

        descriptorLayout->_handle = descriptorLayoutHandle;
        descriptorLayout->_createInfo = createInfo;
        descriptorLayout->_descriptorPool = descriptorPool.ToArcPtr();
        descriptorLayout->SetDebugName(createInfo.Name);

        return descriptorLayout;
    }

    auto CDescriptorLayout::GetHandle() const noexcept -> VkDescriptorSetLayout {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto CDescriptorLayout::GetCreateInfo() const noexcept -> const SDescriptorLayoutCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CDescriptorLayout::GetDescriptorPool() const noexcept -> const CDescriptorPool& {
        RETINA_PROFILE_SCOPED();
        return *_descriptorPool;
    }

    auto CDescriptorLayout::GetDevice() const noexcept -> const CDevice& {
        RETINA_PROFILE_SCOPED();
        return _descriptorPool->GetDevice();
    }

    auto CDescriptorLayout::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
        info.objectHandle = reinterpret_cast<uint64>(GetHandle());
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(
            GetDevice().GetLogger(),
            vkSetDebugUtilsObjectNameEXT(
                GetDevice().GetHandle(),
                &info
            )
        );
    }

    auto CDescriptorLayout::GetBinding(uint32 binding) const noexcept -> const SDescriptorLayoutBinding& {
        RETINA_PROFILE_SCOPED();
        return _createInfo.Bindings[binding];
    }
}
