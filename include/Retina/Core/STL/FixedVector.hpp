#pragma once

#include <Retina/Core/Macros.hpp>
#include <Retina/Core/Types.hpp>

#include <initializer_list>
#include <compare>
#include <array>

namespace Retina::Core {
  template <typename T, usize N>
  class CFixedVector {
  public:
    using StorageType = std::array<T, N>;
    using ValueType = typename StorageType::value_type;
    using SizeType = typename StorageType::size_type;
    using DifferenceType = typename StorageType::difference_type;
    using Pointer = typename StorageType::pointer;
    using ConstPointer = typename StorageType::const_pointer;
    using Reference = typename StorageType::reference;
    using ConstReference = typename StorageType::const_reference;
    using Iterator = typename StorageType::iterator;
    using ConstIterator = typename StorageType::const_iterator;
    using ReverseIterator = typename StorageType::reverse_iterator;
    using ConstReverseIterator = typename StorageType::const_reverse_iterator;

    constexpr CFixedVector() noexcept = default;
    constexpr ~CFixedVector() noexcept = default;
    RETINA_DEFAULT_COPY_MOVE(CFixedVector, constexpr);

    constexpr CFixedVector(std::initializer_list<T> values) noexcept;
    constexpr CFixedVector(usize size) noexcept;
    constexpr CFixedVector(usize size, const T& value = T()) noexcept;

    template <typename I>
    constexpr CFixedVector(I begin, I end) noexcept;

    RETINA_NODISCARD constexpr auto GetSize() const noexcept -> usize;
    RETINA_NODISCARD consteval static auto GetCapacity() noexcept -> usize;

    RETINA_NODISCARD constexpr auto Begin() noexcept -> Iterator;
    RETINA_NODISCARD constexpr auto ConstBegin() const noexcept -> ConstIterator;

    RETINA_NODISCARD constexpr auto End() noexcept -> Iterator;
    RETINA_NODISCARD constexpr auto ConstEnd() const noexcept -> ConstIterator;

    RETINA_NODISCARD constexpr auto ReverseBegin() noexcept -> ReverseIterator;
    RETINA_NODISCARD constexpr auto ConstReverseBegin() const noexcept -> ConstReverseIterator;

    RETINA_NODISCARD constexpr auto ReverseEnd() noexcept -> ReverseIterator;
    RETINA_NODISCARD constexpr auto ConstReverseEnd() const noexcept -> ConstReverseIterator;

    RETINA_NODISCARD constexpr auto GetData() noexcept -> Pointer;
    RETINA_NODISCARD constexpr auto GetData() const noexcept -> ConstPointer;

    RETINA_NODISCARD constexpr auto Front() noexcept -> Reference;
    RETINA_NODISCARD constexpr auto Front() const noexcept -> ConstReference;
    
    RETINA_NODISCARD constexpr auto Back() noexcept -> Reference;
    RETINA_NODISCARD constexpr auto Back() const noexcept -> ConstReference;
    
    RETINA_NODISCARD constexpr auto IsEmpty() const noexcept -> bool;
    RETINA_NODISCARD constexpr auto IsFull() const noexcept -> bool;
    
    RETINA_NODISCARD constexpr auto operator [](usize index) noexcept -> Reference;
    RETINA_NODISCARD constexpr auto operator [](usize index) const noexcept -> ConstReference;

    RETINA_NODISCARD constexpr auto operator <=>(const CFixedVector& other) const noexcept -> std::strong_ordering = default;
    
    template <typename... Args>
    constexpr auto EmplaceBack(Args&&... args) noexcept -> Reference;
    
    constexpr auto PushBack(const T& value) noexcept -> Reference;
    constexpr auto PushBack(T&& value) noexcept -> Reference;
    constexpr auto PopBack() noexcept -> Reference;
    
    constexpr auto Clear() noexcept -> void;
    
  private:
    StorageType _data;
    usize _size = 0;
  };

  template <typename T, usize N>
  constexpr CFixedVector<T, N>::CFixedVector(std::initializer_list<T> values) noexcept {
    RETINA_ASSERT_WITH(values.size() <= N, "Too many values in initializer list");
    for (const auto& value : values) {
      _data[_size++] = value;
    }
  }

  template <typename T, usize N>
  constexpr CFixedVector<T, N>::CFixedVector(usize size) noexcept {
    RETINA_ASSERT_WITH(size <= N, "Size is too large");
    for (usize i = 0; i < size; ++i) {
      _data[_size++] = T();
    }
  }

  template <typename T, usize N>
  constexpr CFixedVector<T, N>::CFixedVector(usize size, const T& value) noexcept {
    RETINA_ASSERT_WITH(size <= N, "Size is too large");
    for (usize i = 0; i < size; ++i) {
      _data[_size++] = value;
    }
  }

  template <typename T, usize N>
  template <typename I>
  constexpr CFixedVector<T, N>::CFixedVector(I begin, I end) noexcept {
    RETINA_ASSERT_WITH(std::distance(begin, end) <= N, "Iterator range is too large");
    while (begin != end) {
      _data[_size++] = *begin++;
    }
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::GetSize() const noexcept -> usize {
    return _size;
  }

  template <typename T, usize N>
  consteval auto CFixedVector<T, N>::GetCapacity() noexcept -> usize {
    return N;
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::Begin() noexcept -> Iterator {
    return _data.begin();
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::ConstBegin() const noexcept -> ConstIterator {
    return _data.cbegin();
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::End() noexcept -> Iterator {
    return _data.end();
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::ConstEnd() const noexcept -> ConstIterator {
    return _data.cend();
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::ReverseBegin() noexcept -> ReverseIterator {
    return _data.rbegin();
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::ConstReverseBegin() const noexcept -> ConstReverseIterator {
    return _data.crbegin();
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::ReverseEnd() noexcept -> ReverseIterator {
    return _data.rend();
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::ConstReverseEnd() const noexcept -> ConstReverseIterator {
    return _data.crend();
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::GetData() noexcept -> Pointer {
    return _data.data();
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::GetData() const noexcept -> ConstPointer {
    return _data.data();
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::Front() noexcept -> Reference {
    return _data.front();
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::Front() const noexcept -> ConstReference {
    return _data.front();
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::Back() noexcept -> Reference {
    return _data.back();
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::Back() const noexcept -> ConstReference {
    return _data.back();
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::IsEmpty() const noexcept -> bool {
    return _size == 0;
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::IsFull() const noexcept -> bool {
    return _size == N;
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::operator [](usize index) noexcept -> Reference {
    return _data[index];
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::operator [](usize index) const noexcept -> ConstReference {
    return _data[index];
  }

  template <typename T, usize N>
  template <typename... Args>
  constexpr auto CFixedVector<T, N>::EmplaceBack(Args&&... args) noexcept -> Reference {
    RETINA_ASSERT_WITH(_size < N, "Vector is full");
    return _data[_size++] = T(std::forward<Args>(args)...);
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::PushBack(const T& value) noexcept -> Reference {
    RETINA_ASSERT_WITH(_size < N, "Vector is full");
    return _data[_size++] = value;
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::PushBack(T&& value) noexcept -> Reference {
    RETINA_ASSERT_WITH(_size < N, "Vector is full");
    return _data[_size++] = std::move(value);
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::PopBack() noexcept -> Reference {
    RETINA_ASSERT_WITH(_size > 0, "Vector is empty");
    return _data[--_size];
  }

  template <typename T, usize N>
  constexpr auto CFixedVector<T, N>::Clear() noexcept -> void {
    _data = {};
    _size = 0;
  }
}
