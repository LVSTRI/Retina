#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/RayTracing/TopLevelAccelerationStructure.hpp>

#include <Retina/Graphics/Buffer.hpp>
#include <Retina/Graphics/DescriptorSetInfo.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/Sampler.hpp>

namespace Retina {
    template <EDescriptorType Type>
    struct SSelectDescriptorType;

    template <>
    struct SSelectDescriptorType<EDescriptorType::E_SAMPLER> {
        using Resource = CSampler;
        using Descriptor = SImageDescriptor;
    };

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

    template <>
    struct SSelectDescriptorType<EDescriptorType::E_ACCELERATION_STRUCTURE_KHR> {
        using Resource = CTopLevelAccelerationStructure;
        using Descriptor = SAccelerationStructureDescriptor;
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
            CArcPtr<CSampler> sampler
        ) noexcept -> uint32
            requires (std::same_as<Resource, CSampler>);

        auto AllocateResource(
            CArcPtr<CBuffer> buffer
        ) noexcept -> uint32
            requires (std::same_as<Resource, CBuffer>);

        auto AllocateResource(
            std::span<CArcPtr<CBuffer>> buffers
        ) noexcept -> std::vector<uint32>
            requires (std::same_as<Resource, CBuffer>);

        auto AllocateResource(
            CArcPtr<CImage> image,
            EImageLayout layout = EImageLayout::E_GENERAL
        ) noexcept -> uint32
            requires (std::same_as<Resource, CImage>);

        auto AllocateResource(
            CArcPtr<CTopLevelAccelerationStructure> accelerationStructure
        ) noexcept -> uint32
            requires (std::same_as<Resource, CTopLevelAccelerationStructure>);

        auto FreeResource(uint32 index) noexcept -> void;

        auto GetWrites() -> std::vector<std::pair<uint32, Descriptor>>&&;

    private:
        RETINA_NODISCARD auto GetFreeIndex() noexcept -> uint32;

        std::vector<CArcPtr<Resource>> _resources;
        std::vector<uint32> _free;
        std::vector<std::pair<uint32, Descriptor>> _writes;
    };
}
