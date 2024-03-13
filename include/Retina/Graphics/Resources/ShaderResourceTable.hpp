#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Resources/ShaderResource.hpp>
#include <Retina/Graphics/Enum.hpp>
#include <Retina/Graphics/Forward.hpp>
#include <Retina/Graphics/TypedBuffer.hpp>

namespace Retina::Graphics {
  constexpr static auto MAX_SAMPLER_RESOURCE_SLOTS = 1024_u32;
  constexpr static auto MAX_BUFFER_RESOURCE_SLOTS = 1048576_u32;
  constexpr static auto MAX_IMAGE_RESOURCE_SLOTS = 262144_u32;
  constexpr static auto MAX_IMAGE_VIEW_RESOURCE_SLOTS = MAX_IMAGE_RESOURCE_SLOTS;

  class CShaderResourceTable {
  public:
    CShaderResourceTable(const CDevice& device) noexcept;
    ~CShaderResourceTable() noexcept = default;
    RETINA_DEFAULT_COPY_MOVE(CShaderResourceTable);

    RETINA_NODISCARD static auto Make(const CDevice& device) noexcept -> std::unique_ptr<CShaderResourceTable>;

    RETINA_NODISCARD auto GetDescriptorSet() const noexcept -> const CDescriptorSet&;
    RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

    RETINA_NODISCARD auto GetDescriptorLayout() const noexcept -> const CDescriptorLayout&;

    template <typename T>
    RETINA_NODISCARD RETINA_INLINE auto MakeBuffer(
      const SBufferCreateInfo& createInfo
    ) noexcept -> CShaderResource<CTypedBuffer<T>>;

    template <typename T>
    RETINA_NODISCARD RETINA_INLINE auto MakeBuffer(
      uint32 count,
      const SBufferCreateInfo& createInfo
    ) noexcept -> std::vector<CShaderResource<CTypedBuffer<T>>>;

  private:
    // TODO: samplers
    Core::CFixedSlotVector<Core::CArcPtr<CBuffer>, MAX_BUFFER_RESOURCE_SLOTS> _buffers;
    Core::CFixedSlotVector<Core::CArcPtr<CImage>, MAX_IMAGE_RESOURCE_SLOTS> _images;
    Core::CFixedSlotVector<Core::CArcPtr<CImageView>, MAX_IMAGE_VIEW_RESOURCE_SLOTS> _imageViews;

    Core::CArcPtr<CDescriptorSet> _descriptorSet;
    Core::CArcPtr<CTypedBuffer<usize>> _addressBuffer;

    Core::CReferenceWrapper<const CDevice> _device;
  };

  template <typename T>
  auto CShaderResourceTable::MakeBuffer(
    const SBufferCreateInfo& createInfo
  ) noexcept -> CShaderResource<CTypedBuffer<T>> {
    RETINA_PROFILE_SCOPED();
    auto buffer = CTypedBuffer<T>::Make(_device, createInfo);
    const auto slot = _buffers.Insert(buffer.template As<CBuffer>());
    _addressBuffer->Write(buffer->GetAddress(), slot);
    return CShaderResource<CTypedBuffer<T>>::Make(*buffer, slot);
  }

  template <typename T>
  RETINA_INLINE auto CShaderResourceTable::MakeBuffer(
    uint32 count,
    const SBufferCreateInfo& createInfo
  ) noexcept -> std::vector<CShaderResource<CTypedBuffer<T>>> {
    RETINA_PROFILE_SCOPED();
    auto buffers = CTypedBuffer<T>::Make(_device, count, createInfo);
    auto resources = std::vector<CShaderResource<CTypedBuffer<T>>>();
    resources.reserve(count);
    for (auto i = 0_u32; i < count; ++i) {
      const auto slot = _buffers.Insert(buffers[i].template As<CBuffer>());
      _addressBuffer->Write(buffers[i]->GetAddress(), slot);
      resources.emplace_back(CShaderResource<CTypedBuffer<T>>::Make(*buffers[i], slot));
    }
    return resources;
  }
}
