#include <Retina/Graphics/Buffer.hpp>
#include <Retina/Graphics/DescriptorSetInfo.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>
#include <Retina/Graphics/Queue.hpp>

#include <volk.h>

#include <algorithm>
#include <cstring>

namespace Retina::Graphics {
  namespace Details {
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

    RETINA_NODISCARD RETINA_INLINE auto GetBufferDeviceAddress(
      const CDevice& device,
      VkBuffer buffer
    ) noexcept -> VkDeviceAddress {
      RETINA_PROFILE_SCOPED();
      auto bufferDeviceAddressInfo = VkBufferDeviceAddressInfo(VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO);
      bufferDeviceAddressInfo.buffer = buffer;
      return vkGetBufferDeviceAddress(device.GetHandle(), &bufferDeviceAddressInfo);
    }
  }

  CBuffer::~CBuffer() noexcept {
    RETINA_PROFILE_SCOPED();
    const auto isSparse = Core::IsFlagEnabled(_createInfo.Flags, EBufferCreateFlag::E_SPARSE_BINDING);
    if (_handle) {
      if (_allocation) {
        vmaDestroyBuffer(_device->GetAllocator(), _handle, _allocation);
        RETINA_GRAPHICS_INFO("Buffer ({}) destroyed", GetDebugName());
      } else if (isSparse) {
        vkDestroyBuffer(_device->GetHandle(), _handle, nullptr);
        RETINA_GRAPHICS_INFO("Buffer ({}) destroyed", GetDebugName());
      }
    }
  }

  auto CBuffer::Make(const CDevice& device, const SBufferCreateInfo& createInfo) noexcept -> Core::CArcPtr<CBuffer> {
    RETINA_PROFILE_SCOPED();
    auto self = Core::CArcPtr(new CBuffer());
    Make(device, createInfo, self.Get());
    return self;
  }

  auto CBuffer::Make(
    const CDevice& device,
    uint32 count,
    const SBufferCreateInfo& createInfo
  ) noexcept -> std::vector<Core::CArcPtr<CBuffer>> {
    RETINA_PROFILE_SCOPED();
    auto buffers = std::vector<Core::CArcPtr<CBuffer>>();
    buffers.reserve(count);
    for (auto i = 0_u32; i < count; ++i) {
      auto currentCreateInfo = createInfo;
      currentCreateInfo.Name += std::to_string(i);
      buffers.push_back(Make(device, currentCreateInfo));
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

  auto CBuffer::GetAllocationInfo() const noexcept -> VmaAllocationInfo {
    RETINA_PROFILE_SCOPED();
    return _allocationInfo;
  }

  auto CBuffer::GetSize() const noexcept -> usize {
    RETINA_PROFILE_SCOPED();
    return _size;
  }

  auto CBuffer::GetAddress() const noexcept -> usize {
    RETINA_PROFILE_SCOPED();
    return _address;
  }

  auto CBuffer::GetCreateInfo() const noexcept -> const SBufferCreateInfo& {
    RETINA_PROFILE_SCOPED();
    return _createInfo;
  }

  auto CBuffer::GetDevice() const noexcept -> const CDevice& {
    RETINA_PROFILE_SCOPED();
    return *_device;
  }

  auto CBuffer::GetDebugName() const noexcept -> std::string_view {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Name;
  }

  auto CBuffer::SetDebugName(std::string_view name) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    RETINA_GRAPHICS_SET_DEBUG_NAME(_device->GetHandle(), _handle, VK_OBJECT_TYPE_BUFFER, name);
    _createInfo.Name = name;
  }

  auto CBuffer::GetCapacity() const noexcept -> usize {
    RETINA_PROFILE_SCOPED();
    return _createInfo.Capacity;
  }

  auto CBuffer::GetAlignment() const noexcept -> usize {
    RETINA_PROFILE_SCOPED();
    return _memoryRequirements.alignment;
  }

  auto CBuffer::GetDescriptor(usize offset, usize size) const noexcept -> SBufferDescriptor {
    RETINA_PROFILE_SCOPED();
    return SBufferDescriptor {
      .Handle = _handle,
      .Memory = _allocationInfo.deviceMemory,
      .Offset = offset,
      .Size = size,
      .Address = _address,
    };
  }

  auto CBuffer::GetData() const noexcept -> uint8* {
    RETINA_PROFILE_SCOPED();
    return static_cast<uint8*>(_allocationInfo.pMappedData);
  }

  auto CBuffer::Clear() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    auto* ptr = GetData();
    if (ptr) {
      std::memset(ptr, 0, GetCapacity());
    }
    _size = 0;
  }

  auto CBuffer::Write(const void* data, usize offset, usize size) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    auto* ptr = GetData();
    if (!ptr) {
      return;
    }
    if (size == 0) {
      return;
    }
    const auto offsetSize = offset + size;
    if (offsetSize > GetCapacity()) {
      RETINA_GRAPHICS_ERROR("Write out of bounds - offset: {}, size: {}, capacity: {}", offset, size, GetCapacity());
      return;
    }
    std::memcpy(ptr + offset, data, size);
    _size = std::max(_size, offsetSize);
  }

  auto CBuffer::Make(const CDevice& device, const SBufferCreateInfo& createInfo, CBuffer* self) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    auto queueFamilyIndices = std::vector {
      device.GetGraphicsQueue().GetFamilyIndex(),
      device.GetTransferQueue().GetFamilyIndex(),
      device.GetComputeQueue().GetFamilyIndex(),
    };
    std::sort(queueFamilyIndices.begin(), queueFamilyIndices.end());
    queueFamilyIndices.erase(std::unique(queueFamilyIndices.begin(), queueFamilyIndices.end()), queueFamilyIndices.end());

    auto bufferCreateInfo = VkBufferCreateInfo(VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
    bufferCreateInfo.flags = AsEnumCounterpart(createInfo.Flags);
    bufferCreateInfo.size = createInfo.Capacity;
    bufferCreateInfo.usage = Details::DEFAULT_BUFFER_USAGE_FLAGS;
    bufferCreateInfo.sharingMode = queueFamilyIndices.size() > 1
      ? VK_SHARING_MODE_CONCURRENT
      : VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = queueFamilyIndices.size();
    bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();

    const auto allocationFlags = [&] -> VmaAllocationCreateFlags {
      const auto isDeviceLocal = Core::IsFlagEnabled(createInfo.Memory, EMemoryPropertyFlag::E_DEVICE_LOCAL);
      const auto isHostVisible = Core::IsFlagEnabled(createInfo.Memory, EMemoryPropertyFlag::E_HOST_VISIBLE);
      const auto isHostCoherent = Core::IsFlagEnabled(createInfo.Memory, EMemoryPropertyFlag::E_HOST_COHERENT);
      const auto isHostCached = Core::IsFlagEnabled(createInfo.Memory, EMemoryPropertyFlag::E_HOST_CACHED);
      auto flags = VmaAllocationCreateFlags();
      if (isDeviceLocal) {
        if (isHostVisible) {
          flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
          if (isHostCoherent) {
            flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
          } else if (isHostCached) {
            flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
          }
        }
      }
      return flags;
    }();

    // TODO: Handle sparse buffers
    auto allocationCreateInfo = VmaAllocationCreateInfo();
    allocationCreateInfo.flags = allocationFlags;
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocationCreateInfo.requiredFlags = AsEnumCounterpart(createInfo.Memory);
    allocationCreateInfo.preferredFlags = {};
    allocationCreateInfo.memoryTypeBits = 0;
    allocationCreateInfo.pool = {};
    allocationCreateInfo.pUserData = {};
    allocationCreateInfo.priority = 1.0f;

    auto bufferHandle = VkBuffer();
    auto allocationHandle = VmaAllocation();
    auto allocationInfo = VmaAllocationInfo();

    RETINA_GRAPHICS_VULKAN_CHECK(
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

    const auto address = Details::GetBufferDeviceAddress(device, bufferHandle);
    RETINA_GRAPHICS_INFO("Buffer ({}) initialized", createInfo.Name);
    RETINA_GRAPHICS_INFO(" - Capacity: {}", createInfo.Capacity);
    RETINA_GRAPHICS_INFO(" - Initial size: {}", size);
    RETINA_GRAPHICS_INFO(" - Address: {}", address);

    self->_handle = bufferHandle;
    self->_allocation = allocationHandle;
    self->_allocationInfo = allocationInfo;
    self->_memoryRequirements = memoryRequirements;
    self->_size = size;
    self->_address = address;
    self->_createInfo = createInfo;
    self->_device = device.ToArcPtr();
    self->SetDebugName(createInfo.Name);
  }
}
