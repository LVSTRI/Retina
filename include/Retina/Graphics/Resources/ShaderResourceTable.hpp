#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Resources/DescriptorTable.hpp>
#include <Retina/Graphics/Buffer.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/TypedBuffer.hpp>

#include <algorithm>
#include <ranges>

namespace Retina {
    template <EDescriptorType D, typename T>
    class CShaderResource {
    public:
        using Self = CShaderResource;
        using Resource = std::conditional_t<std::is_same_v<T, void>, typename SSelectDescriptorType<D>::Resource, T>;
        using Descriptor = SSelectDescriptorType<D>::Descriptor;

        CShaderResource() noexcept = default;
        ~CShaderResource() noexcept = default;

        RETINA_NODISCARD static auto Make(CShaderResourceTable& table, CArcPtr<Resource> resource, uint32 handle) noexcept -> Self;
        
        RETINA_NODISCARD consteval static auto GetDescriptorType() noexcept -> EDescriptorType;
        
        RETINA_NODISCARD auto GetHandle() const noexcept -> uint32;
        RETINA_NODISCARD auto GetResource() noexcept -> Resource&;
        RETINA_NODISCARD auto GetResource() const noexcept -> const Resource&;
        RETINA_NODISCARD auto GetTable() const noexcept -> const CShaderResourceTable&;

        auto Destroy() noexcept -> void;

        RETINA_NODISCARD auto operator *() noexcept -> Resource&;
        RETINA_NODISCARD auto operator *() const noexcept -> const Resource&;

        RETINA_NODISCARD auto operator ->() noexcept -> Resource*;
        RETINA_NODISCARD auto operator ->() const noexcept -> const Resource*;

    private:
        uint32 _handle = 0;

        CArcPtr<Resource> _resource;
        CArcPtr<CShaderResourceTable> _table;
    };

    using SampledImageResource = CShaderResource<EDescriptorType::E_SAMPLED_IMAGE>;
    using StorageImageResource = CShaderResource<EDescriptorType::E_STORAGE_IMAGE>;
    template <typename T>
    using UniformBufferResource = CShaderResource<EDescriptorType::E_UNIFORM_BUFFER, CTypedBuffer<T>>;
    template <typename T>
    using StorageBufferResource = CShaderResource<EDescriptorType::E_STORAGE_BUFFER, CTypedBuffer<T>>;

    class CShaderResourceTable : public IEnableIntrusiveReferenceCount<CShaderResourceTable> {
    public:
        using Self = CShaderResourceTable;

        CShaderResourceTable() noexcept = default;
        ~CShaderResourceTable() noexcept = default;

        RETINA_NODISCARD static auto Make(const CDevice& device) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetSampledImageTable() noexcept -> CDescriptorTable<EDescriptorType::E_SAMPLED_IMAGE>&;
        RETINA_NODISCARD auto GetStorageImageTable() noexcept -> CDescriptorTable<EDescriptorType::E_STORAGE_IMAGE>&;
        RETINA_NODISCARD auto GetUniformBufferTable() noexcept -> CDescriptorTable<EDescriptorType::E_UNIFORM_BUFFER>&;
        RETINA_NODISCARD auto GetStorageBufferTable() noexcept -> CDescriptorTable<EDescriptorType::E_STORAGE_BUFFER>&;

        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;
        RETINA_NODISCARD auto GetDescriptorLayout() const noexcept -> const CDescriptorLayout&;
        RETINA_NODISCARD auto GetDescriptorPool() const noexcept -> const CDescriptorPool&;
        RETINA_NODISCARD auto GetDescriptorSet() const noexcept -> const CDescriptorSet&;

        RETINA_NODISCARD auto MakeSampledImage(const SImageCreateInfo& info) noexcept -> SampledImageResource;
        RETINA_NODISCARD auto MakeStorageImage(const SImageCreateInfo& info) noexcept -> StorageImageResource;

        template <typename T>
        RETINA_NODISCARD auto MakeUniformBuffer(const SBufferCreateInfo& info) noexcept -> UniformBufferResource<T>;
        template <typename T>
        RETINA_NODISCARD auto MakeUniformBuffer(uint32 count, const SBufferCreateInfo& info) noexcept -> std::vector<UniformBufferResource<T>>;
        template <typename T>
        RETINA_NODISCARD auto MakeStorageBuffer(const SBufferCreateInfo& info) noexcept -> StorageBufferResource<T>;
        template <typename T>
        RETINA_NODISCARD auto MakeStorageBuffer(uint32 count, const SBufferCreateInfo& info) noexcept -> std::vector<StorageBufferResource<T>>;

        template <typename C>
        auto FreeResource(const C& resource) noexcept -> void;

        auto Update() noexcept -> void;

    private:
        CDescriptorTable<EDescriptorType::E_SAMPLED_IMAGE> _sampledImageTable;
        CDescriptorTable<EDescriptorType::E_STORAGE_IMAGE> _storageImageTable;
        CDescriptorTable<EDescriptorType::E_UNIFORM_BUFFER> _uniformBufferTable;
        CDescriptorTable<EDescriptorType::E_STORAGE_BUFFER> _storageBufferTable;

        CArcPtr<const CDevice> _device;
        CArcPtr<const CDescriptorSet> _descriptorSet;
    };

    template <typename T>
    auto CShaderResourceTable::MakeUniformBuffer(const SBufferCreateInfo& info) noexcept -> UniformBufferResource<T> {
        RETINA_PROFILE_SCOPED();
        auto buffer = CTypedBuffer<T>::Make(*_device, info);
        const auto index = _uniformBufferTable.AllocateResource(buffer->GetBuffer().ToArcPtr());
        return UniformBufferResource<T>::Make(*this, std::move(buffer), index);
    }

    template <typename T>
    auto CShaderResourceTable::MakeUniformBuffer(
        uint32 count,
        const SBufferCreateInfo& info
    ) noexcept -> std::vector<UniformBufferResource<T>> {
        RETINA_PROFILE_SCOPED();
        auto buffers = CTypedBuffer<T>::Make(*_device, count, info);
        auto bufferHandles = std::vector<CArcPtr<CBuffer>>();
        std::ranges::transform(buffers, std::back_inserter(bufferHandles), [](const auto& buffer) {
            return buffer->GetBuffer().ToArcPtr();
        });
        const auto indices = _uniformBufferTable.AllocateResource(bufferHandles);
        auto resources = std::vector<UniformBufferResource<T>>();
        resources.reserve(count);
        for (auto i = 0_u32; i < count; ++i) {
            resources.emplace_back(UniformBufferResource<T>::Make(*this, std::move(buffers[i]), indices[i]));
        }
        return resources;
    }

    template <typename T>
    auto CShaderResourceTable::MakeStorageBuffer(const SBufferCreateInfo& info) noexcept -> StorageBufferResource<T> {
        RETINA_PROFILE_SCOPED();
        auto buffer = CTypedBuffer<T>::Make(*_device, info);
        const auto index = _storageBufferTable.AllocateResource(buffer->GetBuffer().ToArcPtr());
        return StorageBufferResource<T>::Make(*this, std::move(buffer), index);
    }

    template <typename T>
    auto CShaderResourceTable::MakeStorageBuffer(
        uint32 count,
        const SBufferCreateInfo& info
    ) noexcept -> std::vector<StorageBufferResource<T>> {
        RETINA_PROFILE_SCOPED();
        auto buffers = CTypedBuffer<T>::Make(*_device, count, info);
        auto bufferHandles = std::vector<CArcPtr<CBuffer>>();
        std::ranges::transform(buffers, std::back_inserter(bufferHandles), [](const auto& buffer) {
            return buffer->GetBuffer().ToArcPtr();
        });
        const auto indices = _storageBufferTable.AllocateResource(bufferHandles);
        auto resources = std::vector<StorageBufferResource<T>>();
        resources.reserve(count);
        for (auto i = 0_u32; i < count; ++i) {
            resources.emplace_back(StorageBufferResource<T>::Make(*this, std::move(buffers[i]), indices[i]));
        }
        return resources;
    }

    template <typename C>
    auto CShaderResourceTable::FreeResource(const C& resource) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        switch (C::GetDescriptorType()) {
            case EDescriptorType::E_SAMPLED_IMAGE:
                _sampledImageTable.FreeResource(resource.GetHandle());
                break;
            case EDescriptorType::E_STORAGE_IMAGE:
                _storageImageTable.FreeResource(resource.GetHandle());
                break;
            case EDescriptorType::E_UNIFORM_BUFFER:
                _uniformBufferTable.FreeResource(resource.GetHandle());
                break;
            case EDescriptorType::E_STORAGE_BUFFER:
                _storageBufferTable.FreeResource(resource.GetHandle());
                break;
        }
    }

    template <EDescriptorType D, typename T>
    auto CShaderResource<D, T>::Make(CShaderResourceTable& table, CArcPtr<Resource> resource, uint32 handle) noexcept -> Self {
        RETINA_PROFILE_SCOPED();
        auto shaderResource = Self();
        shaderResource._handle = handle;
        shaderResource._resource = std::move(resource);
        shaderResource._table = table.ToArcPtr();
        return shaderResource;
    }

    template <EDescriptorType D, typename T>
    consteval auto CShaderResource<D, T>::GetDescriptorType() noexcept -> EDescriptorType {
        RETINA_PROFILE_SCOPED();
        return D;
    }

    template <EDescriptorType D, typename T>
    auto CShaderResource<D, T>::GetHandle() const noexcept -> uint32 {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    template <EDescriptorType D, typename T>
    auto CShaderResource<D, T>::GetResource() noexcept -> Resource& {
        RETINA_PROFILE_SCOPED();
        return *_resource;
    }

    template <EDescriptorType D, typename T>
    auto CShaderResource<D, T>::GetResource() const noexcept -> const Resource& {
        RETINA_PROFILE_SCOPED();
        return *_resource;
    }

    template <EDescriptorType D, typename T>
    auto CShaderResource<D, T>::GetTable() const noexcept -> const CShaderResourceTable& {
        RETINA_PROFILE_SCOPED();
        return *_table;
    }

    template <EDescriptorType D, typename T>
    auto CShaderResource<D, T>::Destroy() noexcept -> void {
        RETINA_PROFILE_SCOPED();
        _table->FreeResource(*this);
    }

    template <EDescriptorType D, typename T>
    auto CShaderResource<D, T>::operator *() noexcept -> Resource& {
        RETINA_PROFILE_SCOPED();
        return GetResource();
    }

    template <EDescriptorType D, typename T>
    auto CShaderResource<D, T>::operator *() const noexcept -> const Resource& {
        RETINA_PROFILE_SCOPED();
        return GetResource();
    }

    template <EDescriptorType D, typename T>
    auto CShaderResource<D, T>::operator ->() noexcept -> Resource* {
        RETINA_PROFILE_SCOPED();
        return _resource.Get();
    }

    template <EDescriptorType D, typename T>
    auto CShaderResource<D, T>::operator ->() const noexcept -> const Resource* {
        RETINA_PROFILE_SCOPED();
        return _resource.Get();
    }
}