#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Buffer.hpp>
#include <Retina/Graphics/DescriptorSetInfo.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Image.hpp>

namespace Retina {
    template <EDescriptorType Type>
    struct SSelectDescriptorType;

    template <>
    struct SSelectDescriptorType<EDescriptorType::E_SAMPLED_IMAGE> {
        using Resource = CImage;
        using Descriptor = SImageDescriptor;
    };

    template <>
    struct SSelectDescriptorType<EDescriptorType::E_STORAGE_IMAGE> {
        using Resource = CImage;
        using Descriptor = SImageDescriptor;
    };

    template <>
    struct SSelectDescriptorType<EDescriptorType::E_UNIFORM_BUFFER> {
        using Resource = CBuffer;
        using Descriptor = SBufferDescriptor;
    };

    template <>
    struct SSelectDescriptorType<EDescriptorType::E_STORAGE_BUFFER> {
        using Resource = CBuffer;
        using Descriptor = SBufferDescriptor;
    };

    template <EDescriptorType D>
    class CDescriptorTable {
    public:
        using Self = CDescriptorTable;
        using Resource = SSelectDescriptorType<D>::Resource;
        using Descriptor = SSelectDescriptorType<D>::Descriptor;

        CDescriptorTable() noexcept;
        ~CDescriptorTable() noexcept = default;

        auto AllocateResource(
            CArcPtr<CBuffer> buffer
        ) noexcept -> uint32
            requires (std::same_as<Descriptor, SBufferDescriptor>);

        auto AllocateResource(
            std::span<CArcPtr<CBuffer>> buffers
        ) noexcept -> std::vector<uint32>
            requires (std::same_as<Descriptor, SBufferDescriptor>);

        auto AllocateResource(
            CArcPtr<CImage> image,
            EImageLayout layout = EImageLayout::E_GENERAL
        ) noexcept -> uint32
            requires (std::same_as<Descriptor, SImageDescriptor>);

        auto FreeResource(uint32 index) noexcept -> void;

        auto GetWrites() -> std::vector<std::pair<uint32, Descriptor>>&&;

    private:
        RETINA_NODISCARD auto GetFreeIndex() noexcept -> uint32;

        std::vector<CArcPtr<Resource>> _resources;
        std::vector<uint32> _free;
        std::vector<std::pair<uint32, Descriptor>> _writes;
    };

    template <EDescriptorType D>
    auto CDescriptorTable<D>::AllocateResource(
        CArcPtr<CBuffer> buffer
    ) noexcept -> uint32
        requires (std::same_as<Descriptor, SBufferDescriptor>)
    {
        RETINA_PROFILE_SCOPED();
        const auto index = GetFreeIndex();
        _writes.emplace_back(index, buffer->GetDescriptor());
        _resources[index] = std::move(buffer);
        return index;
    }

    template <EDescriptorType D>
    auto CDescriptorTable<D>::AllocateResource(
        std::span<CArcPtr<CBuffer>> buffers
    ) noexcept -> std::vector<uint32>
        requires (std::same_as<Descriptor, SBufferDescriptor>)
    {
        RETINA_PROFILE_SCOPED();
        auto indices = std::vector<uint32>();
        indices.reserve(buffers.size());
        for (auto&& buffer : buffers) {
            const auto index = GetFreeIndex();
            _writes.emplace_back(index, buffer->GetDescriptor());
            _resources[index] = std::move(buffer);
            indices.emplace_back(index);
        }
        return indices;
    }

    template <EDescriptorType D>
    auto CDescriptorTable<D>::AllocateResource(
        CArcPtr<CImage> image,
        EImageLayout layout
    ) noexcept -> uint32
        requires (std::same_as<Descriptor, SImageDescriptor>)
    {
        RETINA_PROFILE_SCOPED();
        const auto index = GetFreeIndex();
        _writes.emplace_back(index, image->GetDescriptor(layout));
        _resources[index] = std::move(image);
        return index;
    }
}
