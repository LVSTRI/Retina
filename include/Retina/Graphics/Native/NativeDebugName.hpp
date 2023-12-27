#pragma once

#include <Retina/Core/Core.hpp>

#include <string_view>

namespace Retina {
    class INativeDebugName {
    public:
        RETINA_NODISCARD auto GetDebugName() const noexcept -> std::string_view;
        auto SetDebugName(std::string_view name) noexcept -> void;

    protected:
        INativeDebugName() noexcept = default;
        ~INativeDebugName() noexcept = default;

        std::string _name;
    };
}
