#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/BufferInfo.hpp>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <span>

namespace Retina {
    class CBuffer : public INativeDebugName, public IEnableIntrusiveReferenceCount<CBuffer> {
    public:
        using Self = CBuffer;

        CBuffer() noexcept = default;
        ~CBuffer() noexcept;

        RETINA_NODISCARD static auto Make(const CDevice& device, const SBufferCreateInfo& createInfo) noexcept -> CArcPtr<Self>;
        RETINA_NODISCARD static auto Make(
            const CDevice& device,
            uint32 count,
            const SBufferCreateInfo& createInfo
        ) noexcept -> std::vector<CArcPtr<Self>>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkBuffer;
        RETINA_NODISCARD auto GetAllocation() const noexcept -> VmaAllocation;
        RETINA_NODISCARD auto GetAllocationInfo() const noexcept -> const VmaAllocationInfo&;
        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SBufferCreateInfo&;
        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

        auto SetDebugName(std::string_view name) noexcept -> void;

        RETINA_NODISCARD auto GetSize() const noexcept -> uint64;
        RETINA_NODISCARD auto GetAddress() const noexcept -> uint64;
        RETINA_NODISCARD auto GetAlignment() const noexcept -> uint64;
        RETINA_NODISCARD auto GetDescriptor(uint64 offset = 0, uint64 size = Constant::WHOLE_SIZE) const noexcept -> SBufferDescriptor;

        template <typename T>
        auto Write(const T& value, uint64 offset = 0) noexcept -> void;

        template <typename T>
        auto Write(std::span<const T> values, uint64 offset = 0) noexcept -> void;

        auto Write(const void* data, uint64 size, uint64 offset) noexcept -> void;

        template <typename T>
        auto Read(uint64 offset = 0, uint64 size = -1) const noexcept -> std::span<const T>;

    private:
        VkBuffer _handle = {};
        VkMemoryRequirements _memoryRequirements = {};
        VmaAllocation _allocation = {};
        VmaAllocationInfo _allocationInfo = {};

        uint64 _size = 0;
        uint64 _address = 0;

        SBufferCreateInfo _createInfo = {};
        CArcPtr<const CDevice> _device;
    };

    template <typename T>
    auto CBuffer::Write(const T& value, uint64 offset) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        Write<T>({ &value, 1 }, offset);
    }

    template <typename T>
    auto CBuffer::Write(std::span<const T> values, uint64 offset) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        Write(values.data(), values.size_bytes(), offset * sizeof(T));
    }

    template <typename T>
    auto CBuffer::Read(uint64 offset, uint64 size) const noexcept -> std::span<const T> {
        RETINA_PROFILE_SCOPED();
        const auto* mappedPtr = static_cast<const T*>(_allocationInfo.pMappedData);
        if (!mappedPtr) {
            return {};
        }
        // [size] == -1 => whole buffer
        // [size] == 0  => one element
        // otherwise    => [size] elements
        const auto elementSize = _size / sizeof(T);
        if (size == -1_u64) {
            size = elementSize - offset;
        } else if (size == 0_u64) {
            size = 1_u64;
        }
        RETINA_ASSERT_WITH(offset + size >= elementSize, "Read is Out of Bounds");
        return { mappedPtr + offset, size };
    }
}
