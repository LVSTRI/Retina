#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Buffer.hpp>
#include <Retina/Graphics/BufferInfo.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/DescriptorSetInfo.hpp>

namespace Retina {
    template <typename T>
    class CTypedBuffer : public IEnableIntrusiveReferenceCount<CTypedBuffer<T>> {
    public:
        using Self = CTypedBuffer;

        CTypedBuffer() noexcept = default;
        ~CTypedBuffer() noexcept = default;

        RETINA_NODISCARD static auto Make(const CDevice& device, const SBufferCreateInfo& createInfo) noexcept -> CArcPtr<Self>;
        RETINA_NODISCARD static auto Make(
            const CDevice& device,
            uint32 count,
            const SBufferCreateInfo& createInfo
        ) noexcept -> std::vector<CArcPtr<Self>>;
        RETINA_NODISCARD static auto From(CBuffer& buffer) noexcept -> CArcPtr<Self>;
        RETINA_NODISCARD static auto From(std::span<CBuffer> buffers) noexcept -> std::vector<CArcPtr<Self>>;
        RETINA_NODISCARD static auto Upload(const CDevice& device, const SBufferUploadInfo<T>& info) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetBuffer() const noexcept -> CBuffer&;
        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

        RETINA_NODISCARD auto GetCapacity() const noexcept -> uint64;
        RETINA_NODISCARD auto GetSize() const noexcept -> uint64;
        RETINA_NODISCARD auto GetAddress() const noexcept -> uint64;
        RETINA_NODISCARD auto GetAlignment() const noexcept -> uint64;
        RETINA_NODISCARD auto GetDescriptor(uint64 offset = 0, uint64 size = Constant::WHOLE_SIZE) const noexcept -> SBufferDescriptor;

        auto Write(const T& value, uint64 offset = 0) noexcept -> void;
        auto Write(std::span<const T> values, uint64 offset = 0) noexcept -> void;
        auto Write(const void* data, uint64 size, uint64 offset) noexcept -> void;

        auto Read(uint64 offset = 0, uint64 size = -1) const noexcept -> std::span<const T>;

    private:
        CArcPtr<CBuffer> _buffer;
    };

    template <typename T>
    auto CTypedBuffer<T>::Make(const CDevice& device, const SBufferCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto typedBuffer = CArcPtr(new Self());
        typedBuffer->_buffer = CBuffer::Make(device, {
            .Name = createInfo.Name,
            .Flags = createInfo.Flags,
            .Heap = createInfo.Heap,
            .Capacity = createInfo.Capacity * sizeof(T),
        });
        return typedBuffer;
    }

    template <typename T>
    auto CTypedBuffer<T>::Make(
        const CDevice& device,
        uint32 count,
        const SBufferCreateInfo& createInfo
    ) noexcept -> std::vector<CArcPtr<Self>> {
        RETINA_PROFILE_SCOPED();
        auto typedBuffers = std::vector<CArcPtr<Self>>();
        typedBuffers.reserve(count);
        for (auto&& buffer : CBuffer::Make(device, count, {
            .Name = createInfo.Name,
            .Flags = createInfo.Flags,
            .Heap = createInfo.Heap,
            .Capacity = createInfo.Capacity * sizeof(T),
        })) {
            auto typedBuffer = CArcPtr(new Self());
            typedBuffer->_buffer = std::move(buffer);
            typedBuffers.emplace_back(typedBuffer);
        }
        return typedBuffers;
    }

    template <typename T>
    auto CTypedBuffer<T>::From(CBuffer& buffer) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto typedBuffer = CArcPtr(new Self());
        typedBuffer->_buffer = buffer.ToArcPtr();
        return typedBuffer;
    }

    template <typename T>
    auto CTypedBuffer<T>::From(std::span<CBuffer> buffers) noexcept -> std::vector<CArcPtr<Self>> {
        RETINA_PROFILE_SCOPED();
        auto typedBuffers = std::vector<CArcPtr<Self>>();
        typedBuffers.reserve(buffers.size());
        for (auto& buffer : buffers) {
            auto typedBuffer = CArcPtr(new Self());
            typedBuffer->_buffer = buffer.ToArcPtr();
            typedBuffers.emplace_back(typedBuffer);
        }
        return typedBuffers;
    }

    template <typename T>
    auto CTypedBuffer<T>::Upload(const CDevice& device, const SBufferUploadInfo<T>& info) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto typedBuffer = CArcPtr(new Self());
        auto byteView = std::span(
            reinterpret_cast<const uint8*>(info.Data.data()),
            info.Data.size_bytes()
        );
        typedBuffer->_buffer = CBuffer::Upload(device, {
            .Name = info.Name,
            .Data = byteView,
        });
        return typedBuffer;
    }

    template <typename T>
    auto CTypedBuffer<T>::GetBuffer() const noexcept -> CBuffer& {
        RETINA_PROFILE_SCOPED();
        return *_buffer;
    }

    template <typename T>
    auto CTypedBuffer<T>::GetDevice() const noexcept -> const CDevice& {
        RETINA_PROFILE_SCOPED();
        return GetBuffer().GetDevice();
    }

    template <typename T>
    auto CTypedBuffer<T>::GetCapacity() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        const auto& info = _buffer->GetCreateInfo();
        return info.Capacity / sizeof(T);
    }

    template <typename T>
    auto CTypedBuffer<T>::GetSize() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        return _buffer->GetSize() / sizeof(T);
    }

    template <typename T>
    auto CTypedBuffer<T>::GetAddress() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        return _buffer->GetAddress();
    }

    template <typename T>
    auto CTypedBuffer<T>::GetAlignment() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        return _buffer->GetAlignment();
    }

    template <typename T>
    auto CTypedBuffer<T>::GetDescriptor(uint64 offset, uint64 size) const noexcept -> SBufferDescriptor {
        RETINA_PROFILE_SCOPED();
        if (size != Constant::WHOLE_SIZE) {
            size *= sizeof(T);
        }
        return _buffer->GetDescriptor(offset * sizeof(T), size);
    }

    template <typename T>
    auto CTypedBuffer<T>::Write(const T& value, uint64 offset) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        _buffer->Write(value, offset);
    }

    template <typename T>
    auto CTypedBuffer<T>::Write(std::span<const T> values, uint64 offset) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        _buffer->Write(values, offset);
    }

    template <typename T>
    auto CTypedBuffer<T>::Write(const void* data, uint64 size, uint64 offset) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        _buffer->Write(data, size, offset);
    }

    template <typename T>
    auto CTypedBuffer<T>::Read(uint64 offset, uint64 size) const noexcept -> std::span<const T> {
        RETINA_PROFILE_SCOPED();
        return _buffer->Read<T>(offset, size);
    }
}
