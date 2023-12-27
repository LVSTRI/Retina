#pragma once

#include <Retina/Core/Types.hpp>
#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Forwards.hpp>

#include <atomic>

namespace Retina {
    template <typename T>
    class IEnableIntrusiveReferenceCount {
    public:
        using Self = IEnableIntrusiveReferenceCount;

        IEnableIntrusiveReferenceCount(const Self& other) noexcept = delete;
        IEnableIntrusiveReferenceCount(Self&& other) noexcept = delete;
        auto operator =(Self other) noexcept -> Self& = delete;

        RETINA_NODISCARD auto Count() const noexcept -> uint64;
        auto Grab() const noexcept -> uint64;
        auto Drop() const noexcept -> uint64;

        RETINA_NODISCARD auto ToArcPtr() noexcept -> CArcPtr<T>;
        RETINA_NODISCARD auto ToArcPtr() const noexcept -> CArcPtr<const T>;

    protected:
         IEnableIntrusiveReferenceCount() noexcept = default;
        ~IEnableIntrusiveReferenceCount() noexcept = default;

    private:
        mutable std::atomic<uint64> _count = 0;
    };

    template <typename T>
    auto IEnableIntrusiveReferenceCount<T>::Count() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        return _count;
    }

    template <typename T>
    auto IEnableIntrusiveReferenceCount<T>::Grab() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        return ++_count;
    }

    template <typename T>
    auto IEnableIntrusiveReferenceCount<T>::Drop() const noexcept -> uint64 {
        RETINA_PROFILE_SCOPED();
        return --_count;
    }

    template <typename T>
    auto IEnableIntrusiveReferenceCount<T>::ToArcPtr() noexcept -> CArcPtr<T> {
        RETINA_PROFILE_SCOPED();
        return CArcPtr(static_cast<T*>(this));
    }

    template <typename T>
    auto IEnableIntrusiveReferenceCount<T>::ToArcPtr() const noexcept -> CArcPtr<const T> {
        RETINA_PROFILE_SCOPED();
        return CArcPtr(static_cast<const T*>(this));
    }
}