#ifndef ZETA_CONTAINER_INLINED_VECTOR_H
#define ZETA_CONTAINER_INLINED_VECTOR_H

/// @file   container/inlined_vector.h
/// @brief  A `std::vector`-like container with small-buffer optimization.
///
/// `zeta::InlinedVector<T, N>` stores up to `N` elements inline (no heap
/// allocation).  When the size exceeds `N`, elements are moved to the heap.
/// The API mirrors `std::vector`.
///
/// Example:
///   zeta::InlinedVector<int, 8> v = {1, 2, 3};  // no heap allocation
///   v.push_back(4);                               // still inline
///   for (int i = 5; i <= 20; ++i) v.push_back(i); // moves to heap at 9th

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace zeta {

template <typename T, size_t N>
class InlinedVector {
    static_assert(N > 0, "InlinedVector requires N > 0");

    // Inline storage, properly aligned for T
    alignas(T) char inline_buf_[sizeof(T) * N];

    T*     data_     = inline_ptr();
    size_t size_     = 0;
    size_t capacity_ = N;

    bool is_inline() const noexcept { return data_ == inline_ptr(); }
    T*   inline_ptr()       noexcept {
        return reinterpret_cast<T*>(inline_buf_);
    }
    const T* inline_ptr() const noexcept {
        return reinterpret_cast<const T*>(inline_buf_);
    }

    void alloc_heap(size_t new_cap) {
        T* new_data = static_cast<T*>(
            ::operator new(sizeof(T) * new_cap));
        // Move elements from old storage (inline or heap) to new heap
        size_t n = size_;
        for (size_t i = 0; i < n; ++i) {
            ::new (new_data + i) T(std::move(data_[i]));
            data_[i].~T();
        }
        if (!is_inline()) ::operator delete(data_);
        data_ = new_data;
        capacity_ = new_cap;
    }

    void grow() {
        size_t new_cap = capacity_ * 2;
        if (new_cap < 4) new_cap = 4;
        alloc_heap(new_cap);
    }

    void grow_to(size_t required) {
        size_t new_cap = capacity_ * 2;
        if (new_cap < required) new_cap = required;
        alloc_heap(new_cap);
    }

public:
    using value_type      = T;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;
    using iterator        = T*;
    using const_iterator  = const T*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // ── Construction / destruction ────────────────────────────────
    InlinedVector() = default;

    explicit InlinedVector(size_t count, const T& value = T{}) {
        reserve(count);
        for (size_t i = 0; i < count; ++i)
            ::new (data_ + i) T(value);
        size_ = count;
    }

    InlinedVector(std::initializer_list<T> ilist) {
        reserve(ilist.size());
        for (const T& v : ilist)
            ::new (data_ + size_++) T(v);
    }

    ~InlinedVector() {
        clear();
        if (!is_inline()) ::operator delete(data_);
    }

    InlinedVector(const InlinedVector& other) {
        reserve(other.size_);
        for (size_t i = 0; i < other.size_; ++i)
            ::new (data_ + i) T(other.data_[i]);
        size_ = other.size_;
    }

    InlinedVector& operator=(const InlinedVector& other) {
        if (this != &other) {
            clear();
            reserve(other.size_);
            for (size_t i = 0; i < other.size_; ++i)
                ::new (data_ + i) T(other.data_[i]);
            size_ = other.size_;
        }
        return *this;
    }

    InlinedVector(InlinedVector&& other) noexcept {
        if (other.is_inline()) {
            // Move inline elements one by one
            for (size_t i = 0; i < other.size_; ++i)
                ::new (inline_ptr() + i) T(std::move(other.inline_ptr()[i]));
            size_ = other.size_;
            other.destroy_inline();
        } else {
            // Steal heap pointer
            data_     = other.data_;
            size_     = other.size_;
            capacity_ = other.capacity_;
            other.data_     = other.inline_ptr();
            other.size_     = 0;
            other.capacity_ = N;
        }
    }

    InlinedVector& operator=(InlinedVector&& other) noexcept {
        if (this != &other) {
            clear();
            if (!is_inline()) ::operator delete(data_);

            if (other.is_inline()) {
                data_     = inline_ptr();
                capacity_ = N;
                for (size_t i = 0; i < other.size_; ++i)
                    ::new (data_ + i) T(std::move(other.inline_ptr()[i]));
                size_ = other.size_;
                other.destroy_inline();
            } else {
                data_     = other.data_;
                size_     = other.size_;
                capacity_ = other.capacity_;
                other.data_     = other.inline_ptr();
                other.size_     = 0;
                other.capacity_ = N;
            }
        }
        return *this;
    }

    // ── Capacity ──────────────────────────────────────────────────
    size_t size()     const noexcept { return size_; }
    size_t capacity() const noexcept { return capacity_; }
    bool   empty()    const noexcept { return size_ == 0; }
    size_t max_size() const noexcept { return std::numeric_limits<size_t>::max() / sizeof(T); }

    void reserve(size_t n) {
        if (n > capacity_) grow_to(n);
    }

    void shrink_to_fit() {
        if (is_inline()) return;
        if (size_ <= N) {
            // Move back to inline
            T* old_data = data_;
            data_ = inline_ptr();
            for (size_t i = 0; i < size_; ++i) {
                ::new (data_ + i) T(std::move(old_data[i]));
                old_data[i].~T();
            }
            ::operator delete(old_data);
            capacity_ = N;
        } else {
            // Reallocate to exact size
            T* new_data = static_cast<T*>(::operator new(sizeof(T) * size_));
            for (size_t i = 0; i < size_; ++i) {
                ::new (new_data + i) T(std::move(data_[i]));
                data_[i].~T();
            }
            ::operator delete(data_);
            data_ = new_data;
            capacity_ = size_;
        }
    }

    // ── Element access ────────────────────────────────────────────
    T& operator[](size_t i)             noexcept { assert(i < size_); return data_[i]; }
    const T& operator[](size_t i) const noexcept { assert(i < size_); return data_[i]; }

    T& at(size_t i) {
        if (i >= size_) throw std::out_of_range("InlinedVector::at");
        return data_[i];
    }
    const T& at(size_t i) const {
        if (i >= size_) throw std::out_of_range("InlinedVector::at");
        return data_[i];
    }

    T& front()       noexcept { assert(!empty()); return data_[0]; }
    const T& front() const noexcept { assert(!empty()); return data_[0]; }
    T& back()        noexcept { assert(!empty()); return data_[size_ - 1]; }
    const T& back()  const noexcept { assert(!empty()); return data_[size_ - 1]; }

    T* data()             noexcept { return data_; }
    const T* data() const noexcept { return data_; }

    // ── Iterators ─────────────────────────────────────────────────
    iterator       begin()        noexcept { return data_; }
    const_iterator begin()  const noexcept { return data_; }
    const_iterator cbegin() const noexcept { return data_; }
    iterator       end()          noexcept { return data_ + size_; }
    const_iterator end()    const noexcept { return data_ + size_; }
    const_iterator cend()   const noexcept { return data_ + size_; }

    reverse_iterator       rbegin()        noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator(end()); }
    reverse_iterator       rend()          noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend()    const noexcept { return const_reverse_iterator(begin()); }

    // ── Modifiers ─────────────────────────────────────────────────
    void push_back(const T& v) {
        if (size_ >= capacity_) grow();
        ::new (data_ + size_) T(v);
        ++size_;
    }

    void push_back(T&& v) {
        if (size_ >= capacity_) grow();
        ::new (data_ + size_) T(std::move(v));
        ++size_;
    }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (size_ >= capacity_) grow();
        ::new (data_ + size_) T(std::forward<Args>(args)...);
        ++size_;
        return data_[size_ - 1];
    }

    void pop_back() {
        assert(!empty());
        data_[--size_].~T();
    }

    iterator insert(const_iterator pos, const T& v) {
        return emplace(pos, v);
    }

    iterator insert(const_iterator pos, T&& v) {
        return emplace(pos, std::move(v));
    }

    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        size_t idx = static_cast<size_t>(pos - data_);
        if (size_ >= capacity_) {
            size_t new_cap = capacity_ * 2;
            if (new_cap < 4) new_cap = 4;
            T* new_data = static_cast<T*>(::operator new(sizeof(T) * new_cap));
            // Copy elements before idx
            for (size_t i = 0; i < idx; ++i) {
                ::new (new_data + i) T(std::move(data_[i]));
                data_[i].~T();
            }
            // Construct new element
            ::new (new_data + idx) T(std::forward<Args>(args)...);
            // Copy elements after idx
            for (size_t i = idx; i < size_; ++i) {
                ::new (new_data + i + 1) T(std::move(data_[i]));
                data_[i].~T();
            }
            if (!is_inline()) ::operator delete(data_);
            data_ = new_data;
            capacity_ = new_cap;
        } else {
            // Shift elements right
            for (size_t i = size_; i > idx; --i) {
                ::new (data_ + i) T(std::move(data_[i - 1]));
                data_[i - 1].~T();
            }
            data_[idx].~T();
            ::new (data_ + idx) T(std::forward<Args>(args)...);
        }
        ++size_;
        return data_ + idx;
    }

    iterator erase(const_iterator pos) {
        size_t idx = static_cast<size_t>(pos - data_);
        data_[idx].~T();
        for (size_t i = idx; i < size_ - 1; ++i) {
            ::new (data_ + i) T(std::move(data_[i + 1]));
            data_[i + 1].~T();
        }
        --size_;
        return data_ + idx;
    }

    void clear() {
        for (size_t i = 0; i < size_; ++i) data_[i].~T();
        size_ = 0;
    }

    void resize(size_t n, const T& val = T{}) {
        if (n < size_) {
            for (size_t i = n; i < size_; ++i) data_[i].~T();
        } else if (n > size_) {
            reserve(n);
            for (size_t i = size_; i < n; ++i)
                ::new (data_ + i) T(val);
        }
        size_ = n;
    }

    void swap(InlinedVector& other) noexcept(std::is_nothrow_swappable_v<T> &&
                                             std::is_nothrow_move_constructible_v<T>) {
        using std::swap;
        // If both inline, swap element by element
        if (is_inline() && other.is_inline()) {
            size_t common = size_ < other.size_ ? size_ : other.size_;
            for (size_t i = 0; i < common; ++i) swap(data_[i], other.data_[i]);
            for (size_t i = common; i < other.size_; ++i)
                ::new (data_ + i) T(std::move(other.data_[i]));
            for (size_t i = common; i < size_; ++i)
                ::new (other.data_ + i) T(std::move(data_[i]));
            for (size_t i = other.size_; i < size_; ++i) data_[i].~T();
            for (size_t i = size_; i < other.size_; ++i) other.data_[i].~T();
            swap(size_, other.size_);
            return;
        }
        // If one is heap, swap pointers
        swap(data_,     other.data_);
        swap(size_,     other.size_);
        swap(capacity_, other.capacity_);
        // Swap inline buffers
        char tmp[sizeof(T) * N];
        std::memcpy(tmp, inline_buf_, sizeof(inline_buf_));
        std::memcpy(inline_buf_, other.inline_buf_, sizeof(other.inline_buf_));
        std::memcpy(other.inline_buf_, tmp, sizeof(tmp));
    }

    friend void swap(InlinedVector& a, InlinedVector& b) noexcept { a.swap(b); }

    bool operator==(const InlinedVector& other) const {
        return size_ == other.size_ &&
               std::equal(begin(), end(), other.begin());
    }
    bool operator!=(const InlinedVector& other) const { return !(*this == other); }

private:
    void destroy_inline() noexcept {
        for (size_t i = 0; i < size_; ++i) inline_ptr()[i].~T();
        size_ = 0;
    }
};

} // namespace zeta

#endif // ZETA_CONTAINER_INLINED_VECTOR_H
