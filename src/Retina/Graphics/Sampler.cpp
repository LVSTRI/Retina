#include <Retina/Graphics/DescriptorSetInfo.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Sampler.hpp>

#include <volk.h>

namespace Retina {
    CSampler::~CSampler() noexcept {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(_device->GetLogger(), "Destroying Sampler \"{}\"", GetDebugName());
        vkDestroySampler(_device->GetHandle(), _handle, nullptr);
    }

    auto CSampler::Make(const CDevice& device, const SSamplerCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto sampler = CArcPtr(new Self());
        RETINA_LOG_INFO(device.GetLogger(), "Creating Sampler \"{}\"", createInfo.Name);
        auto samplerReductionModeCreateInfo = VkSamplerReductionModeCreateInfo(VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO);
        if (createInfo.ReductionMode) {
            RETINA_LOG_INFO(device.GetLogger(), "- Reduction Mode: {}", ToString(*createInfo.ReductionMode));
            samplerReductionModeCreateInfo.reductionMode = ToEnumCounterpart(*createInfo.ReductionMode);
        }

        RETINA_LOG_INFO(device.GetLogger(), "- Filter: {}", ToString(createInfo.Filter.Min));
        RETINA_LOG_INFO(device.GetLogger(), "- Address: {}", ToString(createInfo.Address.U));
        RETINA_LOG_INFO(device.GetLogger(), "- Mipmap Mode: {}", ToString(createInfo.MipmapMode));

        auto samplerCreateInfo = VkSamplerCreateInfo(VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
        if (createInfo.ReductionMode) {
            samplerCreateInfo.pNext = &samplerReductionModeCreateInfo;
        }
        samplerCreateInfo.magFilter = ToEnumCounterpart(createInfo.Filter.Mag);
        samplerCreateInfo.minFilter = ToEnumCounterpart(createInfo.Filter.Min);
        samplerCreateInfo.mipmapMode = ToEnumCounterpart(createInfo.MipmapMode);
        samplerCreateInfo.addressModeU = ToEnumCounterpart(createInfo.Address.U);
        samplerCreateInfo.addressModeV = ToEnumCounterpart(createInfo.Address.V);
        samplerCreateInfo.addressModeW = ToEnumCounterpart(createInfo.Address.W);
        samplerCreateInfo.mipLodBias = createInfo.LodBias;
        samplerCreateInfo.anisotropyEnable = createInfo.AnisotropyEnable;
        samplerCreateInfo.maxAnisotropy = createInfo.Anisotropy;
        samplerCreateInfo.compareEnable = createInfo.CompareEnable;
        samplerCreateInfo.compareOp = ToEnumCounterpart(createInfo.CompareOperator);
        samplerCreateInfo.minLod = -VK_LOD_CLAMP_NONE;
        samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
        samplerCreateInfo.borderColor = ToEnumCounterpart(createInfo.BorderColor);
        samplerCreateInfo.unnormalizedCoordinates = false;

        auto samplerHandle = VkSampler();
        RETINA_VULKAN_CHECK(device.GetLogger(), vkCreateSampler(device.GetHandle(), &samplerCreateInfo, nullptr, &samplerHandle));

        sampler->_handle = samplerHandle;
        sampler->_device = device.ToArcPtr();
        sampler->SetDebugName(createInfo.Name);

        return sampler;
    }

    auto CSampler::GetHandle() const noexcept -> VkSampler {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto CSampler::GetDevice() const noexcept -> const CDevice& {
        RETINA_PROFILE_SCOPED();
        return *_device;
    }

    auto CSampler::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_SAMPLER;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(_device->GetLogger(), vkSetDebugUtilsObjectNameEXT(_device->GetHandle(), &info));
    }

    auto CSampler::GetDescriptor() const noexcept -> SImageDescriptor {
        RETINA_PROFILE_SCOPED();
        return SImageDescriptor {
            .Sampler = _handle
        };
    }
}
