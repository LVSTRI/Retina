#pragma once

#include <Retina/Core/Types.hpp>
#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Forwards.hpp>

#include <compare>
#include <utility>
#include <atomic>

namespace Retina {
    template <typename T>
    class CArcPtr {
    public:
        using Self = CArcPtr;

        CArcPtr() noexcept;
        ~CArcPtr() noexcept;

        CArcPtr(const Self& other) noexcept;
        CArcPtr(Self&& other) noexcept;
        auto operator =(Self other) noexcept -> Self&;

        CArcPtr(T* ptr) noexcept;
        CArcPtr(std::nullptr_t) noexcept;

        RETINA_NODISCARD auto Get() const noexcept -> T*;
        RETINA_NODISCARD auto Release() noexcept -> T*;
        auto Reset() noexcept -> void;

        template <typename U>
        RETINA_NODISCARD auto As() const noexcept -> U*;

        template <typename C = std::add_const_t<T>, typename..., typename = std::enable_if_t<!std::is_same_v<C, T>>>
        RETINA_NODISCARD auto AsConst() const noexcept -> CArcPtr<C>;

        RETINA_NODISCARD auto operator !() const noexcept -> bool;
        RETINA_NODISCARD auto operator <=>(const Self&) const noexcept -> std::strong_ordering;

        RETINA_NODISCARD auto operator *() const noexcept -> T&;
        RETINA_NODISCARD auto operator ->() const noexcept -> T*;

        RETINA_NODISCARD operator bool() const noexcept;

        template <typename C = std::add_const_t<T>, typename..., typename = std::enable_if_t<!std::is_same_v<C, T>>>
        RETINA_NODISCARD operator CArcPtr<C>() const noexcept;

        template <typename U>
        friend auto swap(CArcPtr<U>& lhs, CArcPtr<U>& rhs) noexcept -> void;

    private:
        T* _ptr = nullptr;
    };

    template <typename T>
    CArcPtr(T*) -> CArcPtr<T>;

    template <typename T>
    CArcPtr(const T*) -> CArcPtr<const T>;

    template <typename T>
    CArcPtr<T>::CArcPtr() noexcept = default;

    template <typename T>
    CArcPtr<T>::~CArcPtr() noexcept {
        RETINA_PROFILE_SCOPED();
        Reset();
    }

    template <typename T>
    CArcPtr<T>::CArcPtr(const Self& other) noexcept : Self(other._ptr) {
        RETINA_PROFILE_SCOPED();
    }

    template <typename T>
    CArcPtr<T>::CArcPtr(Self&& other) noexcept {
        RETINA_PROFILE_SCOPED();
        swap(*this, other);
    }

    template <typename T>
    auto CArcPtr<T>::operator =(Self other) noexcept -> Self& {
        RETINA_PROFILE_SCOPED();
        swap(*this, other);
        return *this;
    }

    template <typename T>
    CArcPtr<T>::CArcPtr(T* ptr) noexcept : _ptr(ptr) {
        RETINA_PROFILE_SCOPED();
        if (_ptr) {
            _ptr->Grab();
        }
    }

    template <typename T>
    CArcPtr<T>::CArcPtr(std::nullptr_t) noexcept : _ptr(nullptr) {
        RETINA_PROFILE_SCOPED();
    }

    template <typename T>
    auto CArcPtr<T>::Get() const noexcept -> T* {
        RETINA_PROFILE_SCOPED();
        return _ptr;
    }

    template <typename T>
    auto CArcPtr<T>::Release() noexcept -> T* {
        RETINA_PROFILE_SCOPED();
        if (!_ptr) {
            return nullptr;
        }
        RETINA_ASSERT_WITH(_ptr->Drop() == 0, "Invalid reference count");
        return std::exchange(_ptr, nullptr);
    }

    template <typename T>
    auto CArcPtr<T>::Reset() noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (_ptr) {
            if (_ptr->Drop() == 0) {
                delete _ptr;
            }
        }
        _ptr = nullptr;
    }

    template <typename T>
    template <typename U>
    auto CArcPtr<T>::As() const noexcept -> U* {
        RETINA_PROFILE_SCOPED();
        return CArcPtr<U>(reinterpret_cast<U*>(_ptr));
    }

    template <typename T>
    template <typename C, typename..., typename>
    auto CArcPtr<T>::AsConst() const noexcept -> CArcPtr<C> {
        RETINA_PROFILE_SCOPED();
        return { static_cast<C*>(_ptr) };
    }

    template <typename T>
    auto CArcPtr<T>::operator !() const noexcept -> bool {
        RETINA_PROFILE_SCOPED();
        return !_ptr;
    }

    template <typename T>
    auto CArcPtr<T>::operator <=>(const Self&) const noexcept -> std::strong_ordering = default;

    template <typename T>
    auto CArcPtr<T>::operator *() const noexcept -> T& {
        RETINA_PROFILE_SCOPED();
        return *_ptr;
    }

    template <typename T>
    auto CArcPtr<T>::operator ->() const noexcept -> T* {
        RETINA_PROFILE_SCOPED();
        return _ptr;
    }

    template <typename T>
    CArcPtr<T>::operator bool() const noexcept {
        RETINA_PROFILE_SCOPED();
        return _ptr;
    }

    template <typename T>
    template <typename C, typename..., typename>
    CArcPtr<T>::operator CArcPtr<C>() const noexcept {
        RETINA_PROFILE_SCOPED();
        return AsConst();
    }

    template <typename U>
    auto swap(CArcPtr<U>& lhs, CArcPtr<U>& rhs) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        using std::swap;
        swap(lhs._ptr, rhs._ptr);
    }
}
