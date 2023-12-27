#include <Retina/Graphics/Buffer.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/DescriptorSetInfo.hpp>
#include <Retina/Graphics/Queue.hpp>

#include <volk.h>

#include <cstring>

namespace Retina {
    constexpr static auto DEFAULT_BUFFER_USAGE_FLAGS =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
        VK_BUFFER_USAGE_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT |
        VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT;

    RETINA_NODISCARD static auto GetBufferDeviceAddress(const CDevice& device, VkBuffer buffer) noexcept -> VkDeviceAddress {
        RETINA_PROFILE_SCOPED();
        auto bufferDeviceAddressInfo = VkBufferDeviceAddressInfo(VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO);
        bufferDeviceAddressInfo.buffer = buffer;
        return vkGetBufferDeviceAddress(device.GetHandle(), &bufferDeviceAddressInfo);
    }

    CBuffer::~CBuffer() noexcept {
        RETINA_PROFILE_SCOPED();
        const auto isSparse = (_createInfo.Flags & EBufferCreateFlag::E_SPARSE_BINDING) == EBufferCreateFlag::E_SPARSE_BINDING;
        RETINA_LOG_INFO(_device->GetLogger(), "Destroying Buffer: \"{}\"", GetDebugName());
        if (_allocation) {
            vmaDestroyBuffer(_device->GetAllocator(), _handle, _allocation);
        } else if (isSparse) {
            vkDestroyBuffer(_device->GetHandle(), _handle, nullptr);
        }
    }

    auto CBuffer::Make(const CDevice& device, const SBufferCreateInfo& createInfo) noexcept -> CArcPtr<Self> {
        RETINA_PROFILE_SCOPED();
        auto buffer = CArcPtr(new Self());
        RETINA_LOG_INFO(device.GetLogger(), "Creating Buffer: \"{}\"", createInfo.Name);
        auto queueFamilyIndices = std::to_array({
            device.GetGraphicsQueue().GetFamilyIndex(),
            device.GetComputeQueue().GetFamilyIndex(),
            device.GetTransferQueue().GetFamilyIndex()
        });
        auto queueFamilyCount = 1_u32;
        if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
            ++queueFamilyCount;
        } else if (queueFamilyIndices[0] != queueFamilyIndices[2]) {
            ++queueFamilyCount;
            std::swap(queueFamilyIndices[1], queueFamilyIndices[2]);
        }
        if (queueFamilyCount > 1 &&
            queueFamilyIndices[0] != queueFamilyIndices[2] &&
            queueFamilyIndices[1] != queueFamilyIndices[2]
        ) {
            ++queueFamilyCount;
        }
        auto bufferCreateInfo = VkBufferCreateInfo(VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
        bufferCreateInfo.flags = ToEnumCounterpart(createInfo.Flags);
        bufferCreateInfo.size = createInfo.Capacity;
        bufferCreateInfo.usage = DEFAULT_BUFFER_USAGE_FLAGS;
        bufferCreateInfo.sharingMode = queueFamilyCount > 1
            ? VK_SHARING_MODE_CONCURRENT
            : VK_SHARING_MODE_EXCLUSIVE;
        bufferCreateInfo.queueFamilyIndexCount = queueFamilyCount;
        bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();

        const auto allocationFlags = [&] {
            if ((createInfo.Heap & Constant::HEAP_TYPE_DEVICE_MAPPABLE) == Constant::HEAP_TYPE_DEVICE_MAPPABLE) {
                return VMA_ALLOCATION_CREATE_MAPPED_BIT |
                       VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            }
            if ((createInfo.Heap & Constant::HEAP_TYPE_HOST_ONLY_COHERENT) == Constant::HEAP_TYPE_HOST_ONLY_COHERENT) {
                return VMA_ALLOCATION_CREATE_MAPPED_BIT |
                       VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            }
            if ((createInfo.Heap & Constant::HEAP_TYPE_HOST_ONLY_CACHED) == Constant::HEAP_TYPE_HOST_ONLY_CACHED) {
                return VMA_ALLOCATION_CREATE_MAPPED_BIT |
                       VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            }
            return VmaAllocationCreateFlagBits();
        }();

        // TODO: Handle sparse buffers
        auto allocationCreateInfo = VmaAllocationCreateInfo();
        allocationCreateInfo.flags = allocationFlags;
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocationCreateInfo.requiredFlags = ToEnumCounterpart(createInfo.Heap);
        allocationCreateInfo.preferredFlags = {};
        allocationCreateInfo.memoryTypeBits = {};
        allocationCreateInfo.pool = {};
        allocationCreateInfo.pUserData = {};
        allocationCreateInfo.priority = 1.0f;

        auto bufferHandle = VkBuffer();
        auto allocationHandle = VmaAllocation();
        auto allocationInfo = VmaAllocationInfo();
        RETINA_VULKAN_CHECK(
            device.GetLogger(),
            vmaCreateBuffer(
                device.GetAllocator(),
                &bufferCreateInfo,
                &allocationCreateInfo,
                &bufferHandle,
                &allocationHandle,
                &allocationInfo
            )
        );

        auto memoryRequirements = VkMemoryRequirements();
        vkGetBufferMemoryRequirements(device.GetHandle(), bufferHandle, &memoryRequirements);

        auto size = 0_u64;
        if (!allocationInfo.pMappedData) {
            size = createInfo.Capacity;
        }

        const auto address = GetBufferDeviceAddress(device, bufferHandle);
        RETINA_LOG_INFO(device.GetLogger(), "- Address: {}", reinterpret_cast<const void*>(address));
        RETINA_LOG_INFO(device.GetLogger(), "- Capacity: {}", createInfo.Capacity);

        buffer->_handle = bufferHandle;
        buffer->_memoryRequirements = memoryRequirements;
        buffer->_allocation = allocationHandle;
        buffer->_allocationInfo = allocationInfo;
        buffer->_size = size;
        buffer->_address = address;
        buffer->_createInfo = createInfo;
        buffer->_device = device.ToArcPtr();
        buffer->SetDebugName(createInfo.Name);

        return buffer;
    }

    auto CBuffer::Make(
        const CDevice& device,
        uint32 count,
        const SBufferCreateInfo& createInfo
    ) noexcept -> std::vector<CArcPtr<Self>> {
        RETINA_PROFILE_SCOPED();
        auto buffers = std::vector<CArcPtr<Self>>();
        buffers.reserve(count);
        for (auto i = 0_u32; i < count; ++i) {
            auto newCreateInfo = createInfo;
            newCreateInfo.Name += std::to_string(i);
            buffers.push_back(Make(device, newCreateInfo));
        }
        return buffers;
    }

    auto CBuffer::GetHandle() const noexcept -> VkBuffer {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto CBuffer::GetAllocation() const noexcept -> VmaAllocation {
        RETINA_PROFILE_SCOPED();
        return _allocation;
    }

    auto CBuffer::GetAllocationInfo() const noexcept -> const VmaAllocationInfo& {
        RETINA_PROFILE_SCOPED();
        return _allocationInfo;
    }

    auto CBuffer::GetCreateInfo() const noexcept -> const SBufferCreateInfo& {
        RETINA_PROFILE_SCOPED();
        return _createInfo;
    }

    auto CBuffer::GetDevice() const noexcept -> const CDevice& {
        RETINA_PROFILE_SCOPED();
        return *_device;
    }

    auto CBuffer::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_BUFFER;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(_device->GetLogger(), vkSetDebugUtilsObjectNameEXT(_device->GetHandle(), &info));
    }

    auto CBuffer::GetSize() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        return _size;
    }

    auto CBuffer::GetAddress() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        return _address;
    }

    auto CBuffer::GetAlignment() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        return _memoryRequirements.alignment;
    }

    auto CBuffer::GetDescriptor(uint64 offset, uint64 size) const noexcept -> SBufferDescriptor {
        RETINA_PROFILE_SCOPED();
        return {
            .Handle = _handle,
            .Memory = _allocationInfo.deviceMemory,
            .Offset = offset,
            .Size = size,
            .Address = _address,
        };
    }

    auto CBuffer::Write(const void* data, uint64 size, uint64 offset) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        auto* mappedPtr = static_cast<uint8*>(_allocationInfo.pMappedData);
        if (!mappedPtr) {
            return;
        }
        if (size == 0) {
            return;
        }
        const auto newSize = offset + size;
        if (newSize > _createInfo.Capacity) {
            RETINA_LOG_ERROR(
                _device->GetLogger(),
                "Write Out of Bounds (offset: {}, size: {}, capacity: {})",
                offset,
                size,
                _createInfo.Capacity
            );
            return;
        }
        std::memcpy(mappedPtr + offset, data, size);
        _size = std::max(_size, newSize);
    }
}
