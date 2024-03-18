#pragma once

#include <Retina/Graphics/Buffer.hpp>
#include <Retina/Graphics/DescriptorSetInfo.hpp>

namespace Retina::Graphics {
  template <typename T>
  class CTypedBuffer : public CBuffer {
  public:
    CTypedBuffer(const CDevice& device) noexcept;
    ~CTypedBuffer() noexcept override = default;

    RETINA_NODISCARD RETINA_INLINE static auto Make(
      const CDevice& device,
      SBufferCreateInfo createInfo
    ) noexcept -> Core::CArcPtr<CTypedBuffer>;

    RETINA_NODISCARD RETINA_INLINE static auto Make(
      const CDevice& device,
      uint32 count,
      const SBufferCreateInfo& createInfo
    ) noexcept -> std::vector<Core::CArcPtr<CTypedBuffer>>;

    RETINA_NODISCARD RETINA_INLINE auto GetBuffer() noexcept -> CBuffer&;
    RETINA_NODISCARD RETINA_INLINE auto GetBuffer() const noexcept -> const CBuffer&;

    RETINA_NODISCARD RETINA_INLINE auto GetSize() const noexcept -> usize;
    RETINA_NODISCARD RETINA_INLINE auto GetCapacity() const noexcept -> usize;

    RETINA_NODISCARD RETINA_INLINE auto GetData() const noexcept -> T*;

    RETINA_NODISCARD RETINA_INLINE auto GetDescriptor(usize offset = 0, usize size = WHOLE_SIZE) const noexcept -> SBufferDescriptor;

    RETINA_INLINE auto Write(const T& value, usize offset = 0) noexcept -> void;
    RETINA_INLINE auto Write(std::span<const T> values, usize offset = 0) noexcept -> void;

    RETINA_NODISCARD RETINA_INLINE auto View(usize offset = 0, usize size = -1_u64) noexcept -> std::span<T>;
    RETINA_NODISCARD RETINA_INLINE auto View(usize offset = 0, usize size = -1_u64) const noexcept -> std::span<const T>;

    RETINA_NODISCARD RETINA_INLINE auto Read(usize offset = 0, usize size = -1_u64) const noexcept -> std::vector<T>;
  };

  template <typename T>
  CTypedBuffer<T>::CTypedBuffer(const CDevice& device) noexcept
    : CBuffer(device)
  {
    RETINA_PROFILE_SCOPED();
  }

  template <typename T>
  auto CTypedBuffer<T>::Make(
    const CDevice& device,
    SBufferCreateInfo createInfo
  ) noexcept -> Core::CArcPtr<CTypedBuffer> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CTypedBuffer(device));
    createInfo.Capacity *= sizeof(T);
    CBuffer::Make(device, createInfo, self.Get());
    return self;
  }

  template <typename T>
  auto CTypedBuffer<T>::Make(
    const CDevice& device,
    uint32 count,
    const SBufferCreateInfo& createInfo
  ) noexcept -> std::vector<Core::CArcPtr<CTypedBuffer>> {
    RETINA_PROFILE_SCOPED();
    auto buffers = std::vector<Core::CArcPtr<CTypedBuffer>>();
    buffers.reserve(count);
    for (auto i = 0_u32; i < count; ++i) {
      auto currentCreateInfo = createInfo;
      currentCreateInfo.Name += std::to_string(i);
      buffers.push_back(Make(device, currentCreateInfo));
    }
    return buffers;
  }

  template <typename T>
  auto CTypedBuffer<T>::GetBuffer() noexcept -> CBuffer& {
    RETINA_PROFILE_SCOPED();
    return static_cast<CBuffer&>(*this);
  }

  template <typename T>
  auto CTypedBuffer<T>::GetBuffer() const noexcept -> const CBuffer& {
    RETINA_PROFILE_SCOPED();
    return static_cast<const CBuffer&>(*this);
  }

  template <typename T>
  auto CTypedBuffer<T>::GetSize() const noexcept -> usize {
    RETINA_PROFILE_SCOPED();
    return CBuffer::GetSizeBytes() / sizeof(T);
  }

  template <typename T>
  auto CTypedBuffer<T>::GetCapacity() const noexcept -> usize {
    RETINA_PROFILE_SCOPED();
    return CBuffer::GetCapacityBytes() / sizeof(T);
  }

  template <typename T>
  auto CTypedBuffer<T>::GetData() const noexcept -> T* {
    RETINA_PROFILE_SCOPED();
    return static_cast<T*>(CBuffer::GetData());
  }

  template <typename T>
  auto CTypedBuffer<T>::GetDescriptor(usize offset, usize size) const noexcept -> SBufferDescriptor {
    RETINA_PROFILE_SCOPED();
    if (size != WHOLE_SIZE) {
      size *= sizeof(T);
    }
    return CBuffer::GetDescriptor(offset * sizeof(T), size);
  }

  template <typename T>
  auto CTypedBuffer<T>::Write(const T& value, usize offset) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    CBuffer::Write<T>(value, offset);
  }

  template <typename T>
  auto CTypedBuffer<T>::Write(std::span<const T> values, usize offset) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    CBuffer::Write<T>(values, offset);
  }

  template <typename T>
  auto CTypedBuffer<T>::View(usize offset, usize size) noexcept -> std::span<T> {
    RETINA_PROFILE_SCOPED();
    return CBuffer::View<T>(offset, size);
  }

  template <typename T>
  auto CTypedBuffer<T>::View(usize offset, usize size) const noexcept -> std::span<const T> {
    RETINA_PROFILE_SCOPED();
    return CBuffer::View<T>(offset, size);
  }

  template <typename T>
  auto CTypedBuffer<T>::Read(usize offset, usize size) const noexcept -> std::vector<T> {
    RETINA_PROFILE_SCOPED();
    return CBuffer::Read<T>(offset, size);
  }
}
