#include <Retina/Graphics/DescriptorSetInfo.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>
#include <Retina/Graphics/Sampler.hpp>

#include <volk.h>

namespace Retina::Graphics {
  CSampler::CSampler(const CDevice& device) noexcept
    : _device(device)
  {
    RETINA_PROFILE_SCOPED();
  }

  CSampler::~CSampler() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      RETINA_GRAPHICS_INFO("Sampler ({}) destroyed", _createInfo.Name);
      vkDestroySampler(_device->GetHandle(), _handle, nullptr);
    }
  }

  auto CSampler::Make(const CDevice& device, const SSamplerCreateInfo& createInfo) noexcept -> Core::CArcPtr<CSampler> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CSampler(device));

    auto samplerReductionModeCreateInfo = VkSamplerReductionModeCreateInfo(VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO);
    if (createInfo.ReductionMode) {
      samplerReductionModeCreateInfo.reductionMode = AsEnumCounterpart(*createInfo.ReductionMode);
    }

    auto samplerCreateInfo = VkSamplerCreateInfo(VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
    samplerCreateInfo.magFilter = AsEnumCounterpart(createInfo.Filter.Mag);
    samplerCreateInfo.minFilter = AsEnumCounterpart(createInfo.Filter.Min);
    samplerCreateInfo.mipmapMode = AsEnumCounterpart(createInfo.MipmapMode);
    samplerCreateInfo.addressModeU = AsEnumCounterpart(createInfo.Address.U);
    samplerCreateInfo.addressModeV = AsEnumCounterpart(createInfo.Address.V);
    samplerCreateInfo.addressModeW = AsEnumCounterpart(createInfo.Address.W);
    samplerCreateInfo.mipLodBias = createInfo.LodBias;
    samplerCreateInfo.anisotropyEnable = createInfo.AnisotropyEnable;
    samplerCreateInfo.maxAnisotropy = createInfo.Anisotropy;
    samplerCreateInfo.compareEnable = createInfo.CompareEnable;
    samplerCreateInfo.compareOp = AsEnumCounterpart(createInfo.CompareOperator);
    samplerCreateInfo.minLod = -VK_LOD_CLAMP_NONE;
    samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerCreateInfo.borderColor = AsEnumCounterpart(createInfo.BorderColor);
    samplerCreateInfo.unnormalizedCoordinates = false;

    if (createInfo.ReductionMode) {
      samplerCreateInfo.pNext = &samplerReductionModeCreateInfo;
    }

    auto samplerHandle = VkSampler();
    RETINA_GRAPHICS_VULKAN_CHECK(vkCreateSampler(device.GetHandle(), &samplerCreateInfo, nullptr, &samplerHandle));

    self->_handle = samplerHandle;
    self->_createInfo = createInfo;
    self->SetDebugName(createInfo.Name);

    RETINA_GRAPHICS_INFO("Sampler ({}) initialized", createInfo.Name);
    RETINA_GRAPHICS_INFO(
      " - Filter: {} - {}",
      ToString(createInfo.Filter.Min),
      ToString(createInfo.Filter.Mag)
    );
    RETINA_GRAPHICS_INFO(
      " - Address: {} - {} - {}",
      ToString(createInfo.Address.U),
      ToString(createInfo.Address.V),
      ToString(createInfo.Address.W)
    );
    RETINA_GRAPHICS_INFO(" - Mipmap Mode: {}", ToString(createInfo.MipmapMode));
    RETINA_GRAPHICS_INFO(" - Border Color: {}", ToString(createInfo.BorderColor));
    RETINA_GRAPHICS_INFO(" - Compare: {}", createInfo.CompareEnable);
    RETINA_GRAPHICS_INFO(" - Anisotropy: {}", createInfo.AnisotropyEnable);
    RETINA_GRAPHICS_INFO(" - LOD Bias: {}", createInfo.LodBias);
    if (createInfo.ReductionMode) {
      RETINA_GRAPHICS_INFO(" - Reduction Mode: {}", ToString(*createInfo.ReductionMode));
    }

    return self;
  }

  auto CSampler::GetHandle() const noexcept -> VkSampler {
    RETINA_PROFILE_SCOPED();
    return _handle;
  }

  auto CSampler::GetCreateInfo() const noexcept -> const SSamplerCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CSampler::GetDevice() const noexcept -> const CDevice& {
    RETINA_PROFILE_SCOPED();
    return _device;
  }

  auto CSampler::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CSampler::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_SET_DEBUG_NAME(_device->GetHandle(), _handle, VK_OBJECT_TYPE_SAMPLER, name);
    _createInfo.Name = name;
  }

  auto CSampler::GetDescriptor() const noexcept -> SImageDescriptor {
    RETINA_PROFILE_SCOPED();
    return SImageDescriptor {
      .Sampler = _handle,
    };
  }
}
