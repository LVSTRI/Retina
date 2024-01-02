#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>

#include <glm/mat4x4.hpp>

namespace Retina {
    class IAccelerationStructure : public INativeDebugName, public IEnableIntrusiveReferenceCount<IAccelerationStructure> {
    public:
        using Self = IAccelerationStructure;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkAccelerationStructureKHR;
        RETINA_NODISCARD auto GetType() const noexcept -> EAccelerationStructureType;
        RETINA_NODISCARD auto GetBuffer() const noexcept -> const CBuffer&;
        RETINA_NODISCARD auto GetAddress() const noexcept -> uint64;
        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

        auto SetDebugName(std::string_view name) noexcept -> void;

        RETINA_NODISCARD auto GetDescriptor() const noexcept -> SAccelerationStructureDescriptor;

    protected:
        IAccelerationStructure(EAccelerationStructureType type) noexcept;
        virtual ~IAccelerationStructure() noexcept;

        VkAccelerationStructureKHR _handle = {};
        EAccelerationStructureType _type = {};
        CArcPtr<CBuffer> _buffer;
        uint64 _address = 0;

        CArcPtr<const CDevice> _device;
    };

    RETINA_NODISCARD auto MakeAccelerationStructureHandle(
        const CDevice& device,
        const CBuffer& buffer,
        EAccelerationStructureType type
    ) noexcept -> VkAccelerationStructureKHR;

    RETINA_NODISCARD auto ToNativeTransformMatrix(glm::mat4 transform) noexcept -> VkTransformMatrixKHR;
}
