#include <Retina/Graphics/Native/NativeDebugName.hpp>

namespace Retina {
    auto INativeDebugName::GetDebugName() const noexcept -> std::string_view {
        RETINA_PROFILE_SCOPED();
        return _name;
    }

    auto INativeDebugName::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        _name = std::string(name);
    }
}
