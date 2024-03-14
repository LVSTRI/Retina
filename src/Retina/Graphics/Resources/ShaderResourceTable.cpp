#include <Retina/Graphics/Resources/ShaderResourceTable.hpp>
#include <Retina/Graphics/Buffer.hpp>
#include <Retina/Graphics/DescriptorLayout.hpp>
#include <Retina/Graphics/DescriptorPool.hpp>
#include <Retina/Graphics/DescriptorSet.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/ImageView.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>

namespace Retina::Graphics {
  CShaderResourceTable::CShaderResourceTable(const CDevice& device) noexcept
    : _device(device)
  {
    RETINA_PROFILE_SCOPED();
  }

  auto CShaderResourceTable::Make(const CDevice& device) noexcept -> std::unique_ptr<CShaderResourceTable> {
    RETINA_PROFILE_SCOPED();
    auto self = std::make_unique<CShaderResourceTable>(device);
    const auto descriptorPoolSizes = std::vector<SDescriptorPoolSize> {
      { EDescriptorType::E_SAMPLER, MAX_SAMPLER_RESOURCE_SLOTS },
      { EDescriptorType::E_SAMPLED_IMAGE, MAX_IMAGE_RESOURCE_SLOTS },
      { EDescriptorType::E_STORAGE_IMAGE, MAX_IMAGE_RESOURCE_SLOTS },
      { EDescriptorType::E_STORAGE_BUFFER, 1 },
    };
    auto descriptorPool = CDescriptorPool::Make(device, SDescriptorPoolCreateInfo {
      .Name = "ShaderResourceTable_DescriptorPool",
      .Flags = EDescriptorPoolCreateFlag::E_UPDATE_AFTER_BIND,
      .MaxSets = 1,
      .PoolSizes = descriptorPoolSizes,
    });

    auto descriptorLayoutBindings = std::vector<SDescriptorLayoutBinding> {
      {
        .Count = MAX_SAMPLER_RESOURCE_SLOTS,
        .Flags =
          EDescriptorBindingFlag::E_UPDATE_UNUSED_WHILE_PENDING |
          EDescriptorBindingFlag::E_PARTIALLY_BOUND,
        .Stages = EShaderStageFlag::E_ALL,
        .Type = EDescriptorType::E_SAMPLER,
      },
      {
        .Count = MAX_IMAGE_RESOURCE_SLOTS,
        .Flags =
          EDescriptorBindingFlag::E_UPDATE_UNUSED_WHILE_PENDING |
          EDescriptorBindingFlag::E_PARTIALLY_BOUND,
        .Stages = EShaderStageFlag::E_ALL,
        .Type = EDescriptorType::E_SAMPLED_IMAGE,
      },
      {
        .Count = MAX_IMAGE_RESOURCE_SLOTS,
        .Flags =
          EDescriptorBindingFlag::E_UPDATE_UNUSED_WHILE_PENDING |
          EDescriptorBindingFlag::E_PARTIALLY_BOUND,
        .Stages = EShaderStageFlag::E_ALL,
        .Type = EDescriptorType::E_STORAGE_IMAGE,
      },
      {
        .Count = 1,
        .Flags =
          EDescriptorBindingFlag::E_UPDATE_UNUSED_WHILE_PENDING |
          EDescriptorBindingFlag::E_PARTIALLY_BOUND,
        .Stages = EShaderStageFlag::E_ALL,
        .Type = EDescriptorType::E_STORAGE_BUFFER,
      },
    };

    auto descriptorLayout = CDescriptorLayout::Make(*descriptorPool, SDescriptorLayoutCreateInfo {
      .Name = "ShaderResourceTable_DescriptorLayout",
      .Bindings = descriptorLayoutBindings,
    });

    auto descriptorSet = CDescriptorSet::Make(*descriptorLayout, {
      .Name = "ShaderResourceTable_DescriptorSet",
    });

    auto addressBuffer = CTypedBuffer<uint64>::Make(device, {
      .Name = "ShaderResourceTable_AddressBuffer",
      .Heap = EHeapType::E_DEVICE_MAPPABLE,
      .Capacity = MAX_BUFFER_RESOURCE_SLOTS,
    });

    descriptorSet->Write(std::to_array({
      SDescriptorWriteInfo {
        .Slot = 0,
        .Type = EDescriptorType::E_STORAGE_BUFFER,
        .Descriptors = std::vector {
          addressBuffer->GetDescriptor(),
        }
      }
    }));

    RETINA_GRAPHICS_INFO("Main shader resource table initialized");

    self->_descriptorSet = std::move(descriptorSet);
    self->_addressBuffer = std::move(addressBuffer);
    return self;
  }

  auto CShaderResourceTable::GetDescriptorSet() const noexcept -> const CDescriptorSet& {
    RETINA_PROFILE_SCOPED();
    return *_descriptorSet;
  }

  auto CShaderResourceTable::GetDevice() const noexcept -> const CDevice& {
    RETINA_PROFILE_SCOPED();
    return _device;
  }

  auto CShaderResourceTable::GetDescriptorLayout() const noexcept -> const CDescriptorLayout& {
    RETINA_PROFILE_SCOPED();
    return _descriptorSet->GetLayout();
  }

  auto CShaderResourceTable::MakeImage(
    const SImageCreateInfo& createInfo,
    EImageLayout layout
  ) noexcept -> CShaderResource<CImage> {
    RETINA_PROFILE_SCOPED();
    auto image = CImage::Make(_device, createInfo);
    const auto descriptor = image->GetDescriptor(layout);
    const auto usage = image->GetUsage();
    const auto slot = _imageSlots.Allocate();
    _imageStorage[slot] = image;

    auto descriptorWriteInfos = std::vector<SDescriptorWriteInfo>();
    descriptorWriteInfos.reserve(2);
    if (Core::IsFlagEnabled(usage, EImageUsageFlag::E_SAMPLED)) {
      auto info = SDescriptorWriteInfo();
      info.Slot = slot;
      info.Type = EDescriptorType::E_SAMPLED_IMAGE;
      info.Descriptors = std::vector { descriptor };
      descriptorWriteInfos.emplace_back(info);
    }
    if (Core::IsFlagEnabled(usage, EImageUsageFlag::E_STORAGE)) {
      auto info = SDescriptorWriteInfo();
      info.Slot = slot;
      info.Type = EDescriptorType::E_STORAGE_IMAGE;
      info.Descriptors = std::vector { descriptor };
      descriptorWriteInfos.emplace_back(info);
    }
    _descriptorSet->Write(descriptorWriteInfos);

    return CShaderResource<CImage>::Make(*image, slot);
  }

  auto CShaderResourceTable::MakeImageView(
    const CImage& image,
    const SImageViewCreateInfo& createInfo,
    EImageLayout layout
  ) noexcept -> CShaderResource<CImageView> {
    RETINA_PROFILE_SCOPED();
    auto imageView = CImageView::Make(image, createInfo);
    const auto descriptor = imageView->GetDescriptor(layout);
    const auto usage = imageView->GetImage().GetUsage();
    const auto slot = _imageSlots.Allocate();
    _imageViewStorage[slot] = imageView;

    auto descriptorWriteInfos = std::vector<SDescriptorWriteInfo>();
    descriptorWriteInfos.reserve(2);
    if (Core::IsFlagEnabled(usage, EImageUsageFlag::E_SAMPLED)) {
      auto info = SDescriptorWriteInfo();
      info.Slot = slot;
      info.Type = EDescriptorType::E_SAMPLED_IMAGE;
      info.Descriptors = std::vector { descriptor };
      descriptorWriteInfos.emplace_back(info);
    }
    if (Core::IsFlagEnabled(usage, EImageUsageFlag::E_STORAGE)) {
      auto info = SDescriptorWriteInfo();
      info.Slot = slot;
      info.Type = EDescriptorType::E_STORAGE_IMAGE;
      info.Descriptors = std::vector { descriptor };
      descriptorWriteInfos.emplace_back(info);
    }
    _descriptorSet->Write(descriptorWriteInfos);

    return CShaderResource<CImageView>::Make(*imageView, slot);
  }
}
