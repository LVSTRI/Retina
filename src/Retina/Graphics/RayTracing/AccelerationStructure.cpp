#include <Retina/Graphics/RayTracing/AccelerationStructure.hpp>

#include <Retina/Graphics/Buffer.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/DescriptorSetInfo.hpp>

#include <volk.h>

#include <cstring>

namespace Retina {
    IAccelerationStructure::IAccelerationStructure(EAccelerationStructureType type) noexcept : _type(type) {
        RETINA_PROFILE_SCOPED();
    }

    IAccelerationStructure::~IAccelerationStructure() noexcept {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(_device->GetLogger(), "Destroying Acceleration Structure: \"{}\"", GetDebugName());
        vkDestroyAccelerationStructureKHR(_device->GetHandle(), _handle, nullptr);
    }

    auto IAccelerationStructure::GetHandle() const noexcept -> VkAccelerationStructureKHR {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto IAccelerationStructure::GetType() const noexcept -> EAccelerationStructureType {
        RETINA_PROFILE_SCOPED();
        return _type;
    }

    auto IAccelerationStructure::GetBuffer() const noexcept -> const CBuffer& {
        RETINA_PROFILE_SCOPED();
        return *_buffer;
    }

    auto IAccelerationStructure::GetAddress() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        return _address;
    }

    auto IAccelerationStructure::GetDevice() const noexcept -> const CDevice& {
        RETINA_PROFILE_SCOPED();
        return *_device;
    }

    auto IAccelerationStructure::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();
        RETINA_VULKAN_CHECK(_device->GetLogger(), vkSetDebugUtilsObjectNameEXT(_device->GetHandle(), &info));
    }

    auto IAccelerationStructure::GetDescriptor() const noexcept -> SAccelerationStructureDescriptor {
        RETINA_PROFILE_SCOPED();
        return {
            .Handle = _handle,
        };
    }

    auto MakeAccelerationStructureHandle(
        const CDevice& device,
        const CBuffer& buffer,
        EAccelerationStructureType type
    ) noexcept -> VkAccelerationStructureKHR {
        RETINA_PROFILE_SCOPED();
        auto createInfo = VkAccelerationStructureCreateInfoKHR(VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR);
        createInfo.buffer = buffer.GetHandle();
        createInfo.size = buffer.GetSize();
        createInfo.type = ToEnumCounterpart(type);
        auto handle = VkAccelerationStructureKHR();
        RETINA_VULKAN_CHECK(device.GetLogger(), vkCreateAccelerationStructureKHR(device.GetHandle(), &createInfo, nullptr, &handle));
        return handle;
    }

    auto ToNativeTransformMatrix(glm::mat4 transform) noexcept -> VkTransformMatrixKHR {
        RETINA_PROFILE_SCOPED();
        auto result = VkTransformMatrixKHR();
        transform = glm::transpose(transform);
        std::memcpy(&result, &transform, sizeof(VkTransformMatrixKHR));
        return result;
    }
}