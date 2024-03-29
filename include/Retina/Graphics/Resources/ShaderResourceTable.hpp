#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Resources/ShaderResource.hpp>
#include <Retina/Graphics/Enum.hpp>
#include <Retina/Graphics/Forward.hpp>
#include <Retina/Graphics/TypedBuffer.hpp>

#include <array>

namespace Retina::Graphics {
  constexpr static auto MAX_SAMPLER_RESOURCE_SLOTS = 1024_u32;
  constexpr static auto MAX_BUFFER_RESOURCE_SLOTS = 1048576_u32;
  constexpr static auto MAX_IMAGE_RESOURCE_SLOTS = 262144_u32;

  class CShaderResourceTable {
  public:
    CShaderResourceTable(const CDevice& device) noexcept;
    ~CShaderResourceTable() noexcept = default;
    RETINA_DEFAULT_COPY_MOVE(CShaderResourceTable);

    RETINA_NODISCARD static auto Make(const CDevice& device) noexcept -> Core::CUniquePtr<CShaderResourceTable>;

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

    RETINA_NODISCARD auto MakeSampler(
      const SSamplerCreateInfo& createInfo
    ) noexcept -> CShaderResource<CSampler>;

    RETINA_NODISCARD auto MakeImage(
      const SImageCreateInfo& createInfo,
      EImageLayout layout = EImageLayout::E_GENERAL
    ) noexcept -> CShaderResource<CImage>;

    RETINA_NODISCARD auto MakeImageView(
      const CImage& image,
      const SImageViewCreateInfo& createInfo,
      EImageLayout layout = EImageLayout::E_GENERAL
    ) noexcept -> CShaderResource<CImageView>;

    auto IsAllocated(CShaderResource<CSampler> handle) const noexcept -> bool;
    template <typename T>
    auto IsAllocated(CShaderResource<CTypedBuffer<T>> handle) const noexcept -> bool;
    auto IsAllocated(CShaderResource<CImage> handle) const noexcept -> bool;
    auto IsAllocated(CShaderResource<CImageView> handle) const noexcept -> bool;

    auto GetSamplerResourceFromHandle(uint32 handle) const noexcept -> CShaderResource<CSampler>;
    template <typename T>
    auto GetBufferResourceFromHandle(uint32 handle) const noexcept -> CShaderResource<CTypedBuffer<T>>;
    auto GetImageResourceFromHandle(uint32 handle) const noexcept -> CShaderResource<CImage>;
    auto GetImageViewResourceFromHandle(uint32 handle) const noexcept -> CShaderResource<CImageView>;

    auto Destroy(CShaderResource<CSampler> handle) noexcept -> void;
    template <typename T>
    auto Destroy(CShaderResource<CTypedBuffer<T>> resource) noexcept -> void;
    auto Destroy(CShaderResource<CImage> handle) noexcept -> void;
    auto Destroy(CShaderResource<CImageView> handle) noexcept -> void;

  private:
    Core::CSlotAllocator<MAX_SAMPLER_RESOURCE_SLOTS> _samplerSlots;
    Core::CSlotAllocator<MAX_BUFFER_RESOURCE_SLOTS> _bufferSlots;
    Core::CSlotAllocator<MAX_IMAGE_RESOURCE_SLOTS> _imageSlots;

    std::array<Core::CArcPtr<CSampler>, MAX_SAMPLER_RESOURCE_SLOTS> _samplerStorage;
    std::array<Core::CArcPtr<CBuffer>, MAX_BUFFER_RESOURCE_SLOTS> _bufferStorage;
    std::array<Core::CArcPtr<CImage>, MAX_IMAGE_RESOURCE_SLOTS> _imageStorage;
    std::array<Core::CArcPtr<CImageView>, MAX_IMAGE_RESOURCE_SLOTS> _imageViewStorage;

    Core::CArcPtr<CDescriptorSet> _descriptorSet;
    Core::CArcPtr<CTypedBuffer<uint64>> _addressBuffer;

    Core::CReferenceWrapper<const CDevice> _device;
  };

  template <typename T>
  auto CShaderResourceTable::MakeBuffer(
    const SBufferCreateInfo& createInfo
  ) noexcept -> CShaderResource<CTypedBuffer<T>> {
    RETINA_PROFILE_SCOPED();
    auto x = std::unique_ptr<T>();
    auto buffer = CTypedBuffer<T>::Make(_device, createInfo);
    const auto slot = _bufferSlots.Allocate();
    _bufferStorage[slot] = buffer;
    _addressBuffer->Write(buffer->GetAddress(), slot);
    return CShaderResource<CTypedBuffer<T>>::Make(*buffer, slot);
  }

  template <typename T>
  auto CShaderResourceTable::MakeBuffer(
    uint32 count,
    const SBufferCreateInfo& createInfo
  ) noexcept -> std::vector<CShaderResource<CTypedBuffer<T>>> {
    RETINA_PROFILE_SCOPED();
    auto buffers = CTypedBuffer<T>::Make(_device, count, createInfo);
    auto resources = std::vector<CShaderResource<CTypedBuffer<T>>>();
    resources.reserve(count);
    for (auto i = 0_u32; i < count; ++i) {
      const auto slot = _bufferSlots.Allocate();
      _bufferStorage[slot] = buffers[i];
      _addressBuffer->Write(buffers[i]->GetAddress(), slot);
      resources.emplace_back(CShaderResource<CTypedBuffer<T>>::Make(*buffers[i], slot));
    }
    return resources;
  }

  template <typename T>
  auto CShaderResourceTable::IsAllocated(CShaderResource<CTypedBuffer<T>> handle) const noexcept -> bool {
    RETINA_PROFILE_SCOPED();
    return handle.IsValid() && _bufferStorage[handle.GetHandle()];
  }

  template <typename T>
  auto CShaderResourceTable::GetBufferResourceFromHandle(uint32 handle) const noexcept -> CShaderResource<CTypedBuffer<T>> {
    RETINA_PROFILE_SCOPED();
    return CShaderResource<T>::Make(*_bufferStorage[handle], handle);
  }

  template <typename T>
  auto CShaderResourceTable::Destroy(CShaderResource<CTypedBuffer<T>> handle) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    if (handle.IsValid()) {
      _bufferSlots.Free(handle.GetHandle());
      _bufferStorage[handle.GetHandle()] = {};
    }
  }
}
