#ifndef ZETA_CONTAINER_FIXED_ARRAY_H
#define ZETA_CONTAINER_FIXED_ARRAY_H

/// @file   container/fixed_array.h
/// @brief  Fixed-size array with small-buffer optimization.
///
/// `zeta::FixedArray<T, InlineElements>` stores a runtime-sized sequence whose
/// length never changes after construction. Small arrays stay inline; larger
/// ones spill to the heap. This mirrors the common Abseil usage pattern for
/// scratch buffers and short-lived staging storage.

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace zeta {

template <typename T, std::size_t InlineElements = 8>
class FixedArray {
    static_assert(InlineElements > 0, "FixedArray requires InlineElements > 0");

public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    FixedArray() noexcept = default;

    explicit FixedArray(size_type count)
        requires std::default_initializable<T>
    {
        InitDefault(count);
    }

    FixedArray(size_type count, const T& value)
        requires std::copy_constructible<T>
    {
        InitFill(count, value);
    }

    FixedArray(std::initializer_list<T> init)
        requires std::copy_constructible<T>
    {
        InitFromRange(init.begin(), init.end());
    }

    FixedArray(const FixedArray& other)
        requires std::copy_constructible<T>
    {
        InitFromRange(other.begin(), other.end());
    }

    FixedArray& operator=(const FixedArray& other)
        requires std::copy_constructible<T>
    {
        if (this != &other) {
            FixedArray tmp(other);
            swap(tmp);
        }
        return *this;
    }

    FixedArray(FixedArray&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        MoveFrom(std::move(other));
    }

    FixedArray& operator=(FixedArray&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (this != &other) {
            Destroy();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    ~FixedArray() {
        Destroy();
    }

    [[nodiscard]] size_type size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] pointer data() noexcept { return data_; }
    [[nodiscard]] const_pointer data() const noexcept { return data_; }

    [[nodiscard]] reference operator[](size_type i) noexcept {
        assert(i < size_);
        return data_[i];
    }

    [[nodiscard]] const_reference operator[](size_type i) const noexcept {
        assert(i < size_);
        return data_[i];
    }

    [[nodiscard]] reference at(size_type i) {
        if (i >= size_) throw std::out_of_range("FixedArray::at");
        return data_[i];
    }

    [[nodiscard]] const_reference at(size_type i) const {
        if (i >= size_) throw std::out_of_range("FixedArray::at");
        return data_[i];
    }

    [[nodiscard]] reference front() noexcept {
        assert(!empty());
        return data_[0];
    }

    [[nodiscard]] const_reference front() const noexcept {
        assert(!empty());
        return data_[0];
    }

    [[nodiscard]] reference back() noexcept {
        assert(!empty());
        return data_[size_ - 1];
    }

    [[nodiscard]] const_reference back() const noexcept {
        assert(!empty());
        return data_[size_ - 1];
    }

    [[nodiscard]] iterator begin() noexcept { return data_; }
    [[nodiscard]] const_iterator begin() const noexcept { return data_; }
    [[nodiscard]] const_iterator cbegin() const noexcept { return data_; }
    [[nodiscard]] iterator end() noexcept { return data_ + size_; }
    [[nodiscard]] const_iterator end() const noexcept { return data_ + size_; }
    [[nodiscard]] const_iterator cend() const noexcept { return data_ + size_; }
    [[nodiscard]] reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    [[nodiscard]] reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    [[nodiscard]] const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    [[nodiscard]] const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    void fill(const T& value)
        requires std::is_copy_assignable_v<T>
    {
        std::fill(begin(), end(), value);
    }

    void swap(FixedArray& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (this == &other) {
            return;
        }
        FixedArray tmp(std::move(other));
        other = std::move(*this);
        *this = std::move(tmp);
    }

    friend void swap(FixedArray& a, FixedArray& b) noexcept(noexcept(a.swap(b))) {
        a.swap(b);
    }

    [[nodiscard]] bool operator==(const FixedArray& other) const {
        return size_ == other.size_ && std::equal(begin(), end(), other.begin());
    }

    [[nodiscard]] bool operator!=(const FixedArray& other) const {
        return !(*this == other);
    }

private:
    alignas(T) unsigned char inline_storage_[sizeof(T) * InlineElements];
    pointer data_ = InlinePtr();
    size_type size_ = 0;

    [[nodiscard]] pointer InlinePtr() noexcept {
        return reinterpret_cast<pointer>(inline_storage_);
    }

    [[nodiscard]] const_pointer InlinePtr() const noexcept {
        return reinterpret_cast<const_pointer>(inline_storage_);
    }

    [[nodiscard]] bool IsInline() const noexcept {
        return data_ == InlinePtr();
    }

    void Allocate(size_type count) {
        if (count <= InlineElements) {
            data_ = InlinePtr();
        } else {
            data_ = static_cast<pointer>(::operator new(sizeof(T) * count));
        }
    }

    void InitDefault(size_type count) {
        Allocate(count);
        size_type i = 0;
        try {
            for (; i < count; ++i) {
                ::new (data_ + i) T();
            }
            size_ = count;
        } catch (...) {
            DestroyConstructed(i);
            if (!IsInline()) {
                ::operator delete(data_);
                data_ = InlinePtr();
            }
            throw;
        }
    }

    void InitFill(size_type count, const T& value) {
        Allocate(count);
        size_type i = 0;
        try {
            for (; i < count; ++i) {
                ::new (data_ + i) T(value);
            }
            size_ = count;
        } catch (...) {
            DestroyConstructed(i);
            if (!IsInline()) {
                ::operator delete(data_);
                data_ = InlinePtr();
            }
            throw;
        }
    }

    template <typename It>
    void InitFromRange(It first, It last) {
        const size_type count = static_cast<size_type>(std::distance(first, last));
        Allocate(count);
        size_type i = 0;
        try {
            for (; first != last; ++first, ++i) {
                ::new (data_ + i) T(*first);
            }
            size_ = count;
        } catch (...) {
            DestroyConstructed(i);
            if (!IsInline()) {
                ::operator delete(data_);
                data_ = InlinePtr();
            }
            throw;
        }
    }

    void MoveFrom(FixedArray&& other) {
        if (other.IsInline()) {
            data_ = InlinePtr();
            size_type i = 0;
            try {
                for (; i < other.size_; ++i) {
                    ::new (data_ + i) T(std::move(other.data_[i]));
                }
                size_ = other.size_;
            } catch (...) {
                DestroyConstructed(i);
                throw;
            }
            other.DestroyConstructed(other.size_);
            other.size_ = 0;
        } else {
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = other.InlinePtr();
            other.size_ = 0;
        }
    }

    void DestroyConstructed(size_type count) noexcept {
        for (size_type i = 0; i < count; ++i) {
            data_[i].~T();
        }
    }

    void Destroy() noexcept {
        DestroyConstructed(size_);
        if (!IsInline()) {
            ::operator delete(data_);
        }
        data_ = InlinePtr();
        size_ = 0;
    }
};

} // namespace zeta

#endif // ZETA_CONTAINER_FIXED_ARRAY_H
