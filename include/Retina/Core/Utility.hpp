#pragma once

#include <Retina/Core/Core.hpp>

#include <cstdlib>

namespace Retina {
    template <typename... Ts>
    RETINA_NODISCARD consteval auto SizeBytesOfPack() noexcept -> uint64 {
        return (sizeof(Ts) + ...);
    }

    template <typename T>
    RETINA_NODISCARD constexpr auto AsConstPtr(const T& value) noexcept -> const T* {
        return &value;
    }

    template <typename T>
    RETINA_NODISCARD constexpr auto AsConstPtr(const T(&value)[]) noexcept -> const T* {
        return &value[0];
    }

    template <typename T>
    RETINA_NODISCARD constexpr auto AsConstPtr(const T&& value) noexcept -> const T* {
        return &value;
    }

    template <typename T>
    RETINA_NODISCARD constexpr auto AsConstPtr(const T(&&value)[]) noexcept -> const T* {
        return &value[0];
    }

    template <typename T>
    RETINA_NODISCARD constexpr auto AsMutPtr(T&& value) noexcept -> T* {
        return &value;
    }

    template <typename T>
    RETINA_NODISCARD constexpr auto AsMutPtr(T(&&value)[]) noexcept -> T* {
        return &value[0];
    }

    template <typename T>
    RETINA_NODISCARD constexpr auto AsConstRef(const T(&value)[]) noexcept -> const T& {
        return *AsConstPtr(value);
    }

    template <typename T>
    RETINA_NODISCARD constexpr auto AsConstRef(const T&& value) noexcept -> const T& {
        return *AsConstPtr(value);
    }

    template <typename T>
    RETINA_NODISCARD constexpr auto AsConstRef(const T(&&value)[]) noexcept -> const T& {
        return *AsConstPtr(value);
    }

    template <typename T>
    RETINA_NODISCARD constexpr auto AsMutRef(T&& value) noexcept -> T& {
        return *AsMutPtr(value);
    }

    template <typename T>
    RETINA_NODISCARD constexpr auto AsMutRef(T(&&value)[]) noexcept -> T& {
        return *AsMutPtr(value);
    }

    template <typename... Ts>
    RETINA_NODISCARD constexpr auto MakeByteArray(Ts&&... values) noexcept -> std::array<uint8, SizeBytesOfPack<Ts...>()> {
        auto output = std::array<uint8, SizeBytesOfPack<Ts...>()>();
        auto offset = 0_u64;
        ((__builtin_memcpy(&output[offset], &values, sizeof(Ts)), offset += sizeof(Ts)), ...);
        return output;
    }

    RETINA_NODISCARD constexpr auto MultiByteStringToWide(std::string_view input) noexcept -> std::wstring {
        auto output = std::wstring(input.size(), L'\0');
        std::mbstowcs(output.data(), input.data(), input.size());
        return output;
    }
}