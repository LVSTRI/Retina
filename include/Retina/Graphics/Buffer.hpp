#pragma once

#include <Retina/Graphics/BufferInfo.hpp>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <span>

namespace Retina::Graphics {
  class CBuffer : public Core::IEnableIntrusiveReferenceCount<CBuffer> {
  public:
    CBuffer(const CDevice& device) noexcept;
    virtual ~CBuffer() noexcept;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      const SBufferCreateInfo& createInfo
    ) noexcept -> Core::CArcPtr<CBuffer>;

    RETINA_NODISCARD static auto Make(
      const CDevice& device,
      uint32 count,
      const SBufferCreateInfo& createInfo
    ) noexcept -> std::vector<Core::CArcPtr<CBuffer>>;

    RETINA_NODISCARD auto GetHandle() const noexcept -> VkBuffer;
    RETINA_NODISCARD auto GetAllocation() const noexcept -> VmaAllocation;
    RETINA_NODISCARD auto GetAllocationInfo() const noexcept -> VmaAllocationInfo;

    RETINA_NODISCARD auto GetSizeBytes() const noexcept -> usize;
    RETINA_NODISCARD auto GetAddress() const noexcept -> usize;

    RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SBufferCreateInfo&;
    RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

    RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
    auto SetDebugName(std::string_view name) noexcept -> void;

    RETINA_NODISCARD auto GetCapacity() const noexcept -> usize;
    RETINA_NODISCARD auto GetAlignment() const noexcept -> usize;
    RETINA_NODISCARD auto GetDescriptor(usize offset = 0, usize size = WHOLE_SIZE) const noexcept -> SBufferDescriptor;
    RETINA_NODISCARD auto GetData() const noexcept -> uint8*;

    auto Clear() noexcept -> void;

    auto Write(const void* data, usize offset, usize size) noexcept -> void;

    template <typename T>
    RETINA_INLINE auto Write(const T& value, usize offset = 0) noexcept -> void;
    template <typename T>
    RETINA_INLINE auto Write(std::span<const T> values, usize offset = 0) noexcept -> void;

    template <typename T>
    RETINA_NODISCARD RETINA_INLINE auto View(usize offset = 0, usize size = -1_u64) noexcept -> std::span<T>;
    template <typename T>
    RETINA_NODISCARD RETINA_INLINE auto View(usize offset = 0, usize size = -1_u64) const noexcept -> std::span<const T>;

    template <typename T>
    RETINA_NODISCARD RETINA_INLINE auto Read(usize offset = 0, usize size = -1_u64) const noexcept -> std::vector<T>;

  protected:
    static auto Make(const CDevice& device, const SBufferCreateInfo& createInfo, CBuffer* self) noexcept -> void;

  protected:
    VkBuffer _handle = {};
    VmaAllocation _allocation = {};
    VmaAllocationInfo _allocationInfo = {};
    VkMemoryRequirements _memoryRequirements = {};

    usize _size = 0;
    usize _address = 0;

    SBufferCreateInfo _createInfo = {};
    Core::CReferenceWrapper<const CDevice> _device;
  };

  template <typename T>
  auto CBuffer::Write(const T& value, usize offset) noexcept -> void {
    Write<T>({ &value, 1 }, offset);
  }

  template <typename T>
  auto CBuffer::Write(std::span<const T> values, usize offset) noexcept -> void {
    Write(values.data(), offset * sizeof(T), values.size_bytes());
  }

  template <typename T>
  auto CBuffer::View(usize offset, usize size) noexcept -> std::span<T> {
    auto* ptr = static_cast<T*>(_allocationInfo.pMappedData);
    if (!ptr) {
      return {};
    }
    if (size == -1_u64) {
      size = std::max(_size / sizeof(T) - offset, 0_u64);
    }
    return { ptr + offset, size };
  }


  template <typename T>
  auto CBuffer::View(usize offset, usize size) const noexcept -> std::span<const T> {
    return const_cast<CBuffer*>(this)->View<const T>(offset, size);
  }

  template <typename T>
  auto CBuffer::Read(usize offset, usize size) const noexcept -> std::vector<T> {
    const auto view = View<T>(offset, size);
    return { view.begin(), view.end() };
  }
}
