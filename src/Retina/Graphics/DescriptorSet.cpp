#include <Retina/Graphics/DescriptorPool.hpp>
#include <Retina/Graphics/DescriptorLayout.hpp>
#include <Retina/Graphics/DescriptorSet.hpp>
#include <Retina/Graphics/Device.hpp>

#include <volk.h>

namespace Retina {
    CDescriptorSet::~CDescriptorSet() noexcept {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(GetDevice().GetLogger(), "Destroying Descriptor Set: \"{}\"", GetDebugName());
        vkFreeDescriptorSets(
            GetDevice().GetHandle(),
            GetDescriptorPool().GetHandle(),
            1,
            &_handle
        );
    }

    auto CDescriptorSet::Make(const CDevice& device, const SDescriptorSetCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto descriptorSet = CArcPtr(new Self());
        RETINA_LOG_INFO(device.GetLogger(), "Creating Descriptor Set: \"{}\"", createInfo.Name);
        const auto& descriptorLayout = *createInfo.Layout;
        const auto& descriptorPool = descriptorLayout.GetDescriptorPool();
        const auto& descriptorLayoutHandle = descriptorLayout.GetHandle();

        RETINA_LOG_INFO(device.GetLogger(), "- Layout: {}", descriptorLayout.GetDebugName());
        RETINA_LOG_INFO(device.GetLogger(), "- Descriptor Pool: {}", descriptorPool.GetDebugName());

        auto allocateInfo = VkDescriptorSetAllocateInfo(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
        allocateInfo.descriptorPool = descriptorPool.GetHandle();
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &descriptorLayoutHandle;

        auto descriptorSetHandle = VkDescriptorSet();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vkAllocateDescriptorSets(
                device.GetHandle(),
                &allocateInfo,
                &descriptorSetHandle
            )
        );
        descriptorSet->_handle = descriptorSetHandle;
        descriptorSet->_createInfo = createInfo;
        descriptorSet->SetDebugName(createInfo.Name);

        return descriptorSet;
    }

    auto CDescriptorSet::Make(
        const CDevice& device,
        uint32 count,
        const SDescriptorSetCreateInfo& createInfo
    ) noexcept -> std::vector<CArcPtr<Self>> {
        RETINA_PROFILE_SCOPED();
        auto descriptorSets = std::vector<CArcPtr<Self>>();
        descriptorSets.reserve(count);
        for (auto i = 0_u32; i < count; ++i) {
            auto newCreateInfo = createInfo;
            newCreateInfo.Name += std::to_string(i);
            descriptorSets.push_back(Make(device, newCreateInfo));
        }
        return descriptorSets;
    }

    auto CDescriptorSet::Make(
        const CDevice& device,
        std::span<const SDescriptorSetCreateInfo> createInfos
    ) noexcept -> std::vector<CArcPtr<Self>> {
        RETINA_PROFILE_SCOPED();
        auto descriptorSets = std::vector<CArcPtr<Self>>();
        descriptorSets.reserve(createInfos.size());
        for (auto i = 0_u32; i < createInfos.size(); ++i) {
            auto newCreateInfo = createInfos[i];
            newCreateInfo.Name += std::to_string(i);
            descriptorSets.push_back(Make(device, newCreateInfo));
        }
        return descriptorSets;
    }

    auto CDescriptorSet::GetHandle() const noexcept -> VkDescriptorSet {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto CDescriptorSet::GetLayout() const noexcept -> const CDescriptorLayout& {
        RETINA_PROFILE_SCOPED();
        return *_createInfo.Layout;
    }

    auto CDescriptorSet::GetCreateInfo() const noexcept -> const SDescriptorSetCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CDescriptorSet::GetDevice() const noexcept -> const CDevice& {
        RETINA_PROFILE_SCOPED();
        return GetLayout().GetDevice();
    }

    auto CDescriptorSet::GetDescriptorPool() const noexcept -> const CDescriptorPool& {
        RETINA_PROFILE_SCOPED();
        return GetLayout().GetDescriptorPool();
    }

    auto CDescriptorSet::Write(std::span<const SDescriptorWriteInfo> writes) const noexcept -> void {
        RETINA_PROFILE_SCOPED();
        auto descriptorWrites = std::vector<VkWriteDescriptorSet>();
        descriptorWrites.reserve(writes.size());
        auto descriptorBufferInfos = std::vector<std::vector<VkDescriptorBufferInfo>>();
        descriptorBufferInfos.reserve(writes.size());
        auto descriptorImageInfos = std::vector<std::vector<VkDescriptorImageInfo>>();
        descriptorImageInfos.reserve(writes.size());

        for (const auto& currentWrite : writes) {
            auto write = VkWriteDescriptorSet(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
            write.dstSet = _handle;
            write.dstBinding = currentWrite.Binding;
            if (currentWrite.Binding == -1_u32) {
                write.dstBinding = *GetLayout().FindBindingFromDescriptorType(currentWrite.Type);
            }

            write.dstArrayElement = currentWrite.Slot;
            write.descriptorType = ToEnumCounterpart(currentWrite.Type);

            switch (currentWrite.Type) {
                case EDescriptorType::E_SAMPLER: RETINA_FALLTHROUGH;
                case EDescriptorType::E_COMBINED_IMAGE_SAMPLER: RETINA_FALLTHROUGH;
                case EDescriptorType::E_SAMPLED_IMAGE: RETINA_FALLTHROUGH;
                case EDescriptorType::E_STORAGE_IMAGE: RETINA_FALLTHROUGH;
                case EDescriptorType::E_INPUT_ATTACHMENT: {
                    const auto& imageDescriptors = std::get<std::vector<SImageDescriptor>>(currentWrite.Descriptors);
                    auto imageInfos = std::vector<VkDescriptorImageInfo>();
                    imageInfos.reserve(imageDescriptors.size());
                    for (const auto& imageDescriptor : imageDescriptors) {
                        auto imageInfo = VkDescriptorImageInfo();
                        imageInfo.sampler = imageDescriptor.Sampler;
                        imageInfo.imageView = imageDescriptor.View;
                        imageInfo.imageLayout = ToEnumCounterpart(imageDescriptor.Layout);
                        imageInfos.emplace_back(imageInfo);
                    }
                    write.descriptorCount = static_cast<uint32>(imageInfos.size());
                    write.pImageInfo = imageInfos.data();
                    descriptorWrites.emplace_back(write);
                    descriptorImageInfos.emplace_back(std::move(imageInfos));
                } break;

                case EDescriptorType::E_UNIFORM_TEXEL_BUFFER: RETINA_FALLTHROUGH;
                case EDescriptorType::E_STORAGE_TEXEL_BUFFER:
                    RETINA_PANIC_WITH(GetDevice().GetLogger(), "Texel Buffers are not supported");

                case EDescriptorType::E_UNIFORM_BUFFER: RETINA_FALLTHROUGH;
                case EDescriptorType::E_STORAGE_BUFFER: RETINA_FALLTHROUGH;
                case EDescriptorType::E_UNIFORM_BUFFER_DYNAMIC: RETINA_FALLTHROUGH;
                case EDescriptorType::E_STORAGE_BUFFER_DYNAMIC: {
                    const auto& bufferDescriptors = std::get<std::vector<SBufferDescriptor>>(currentWrite.Descriptors);
                    auto bufferInfos = std::vector<VkDescriptorBufferInfo>();
                    bufferInfos.reserve(bufferDescriptors.size());
                    for (const auto& bufferDescriptor : bufferDescriptors) {
                        auto bufferInfo = VkDescriptorBufferInfo();
                        bufferInfo.buffer = bufferDescriptor.Handle;
                        bufferInfo.offset = bufferDescriptor.Offset;
                        bufferInfo.range = bufferDescriptor.Size;
                        bufferInfos.emplace_back(bufferInfo);
                    }
                    write.descriptorCount = static_cast<uint32>(bufferInfos.size());
                    write.pBufferInfo = bufferInfos.data();
                    descriptorWrites.emplace_back(write);
                    descriptorBufferInfos.emplace_back(std::move(bufferInfos));
                } break;

                case EDescriptorType::E_ACCELERATION_STRUCTURE_KHR:
                    RETINA_PANIC_WITH(GetDevice().GetLogger(), "Acceleration Structures are not supported");

                default:
                    RETINA_PANIC_WITH(GetDevice().GetLogger(), "Unknown Descriptor Type");
            }
        }

        if (!descriptorWrites.empty()) {
            vkUpdateDescriptorSets(
                GetDevice().GetHandle(),
                static_cast<uint32>(descriptorWrites.size()),
                descriptorWrites.data(),
                0,
                nullptr
            );
        }
    }

    auto CDescriptorSet::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(
            GetDevice().GetLogger(),
            vkSetDebugUtilsObjectNameEXT(
                GetDevice().GetHandle(),
                &info
            )
        );
    }
}
