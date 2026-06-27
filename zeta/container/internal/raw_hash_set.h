#ifndef ZETA_CONTAINER_INTERNAL_RAW_HASH_SET_H
#define ZETA_CONTAINER_INTERNAL_RAW_HASH_SET_H

/// @file   container/internal/raw_hash_set.h
/// @brief  Swiss Table: open-addressing hash table with SIMD probing.
///
/// Production-quality implementation supporting:
///   - NEON (ARM64) / SSE2 (x86-64) / scalar fallback
///   - Heterogeneous lookup (transparent hash / equal)
///   - Proper const-correctness with distinct const_iterator
///   - try_emplace, insert_or_assign, reserve, swap
///   - Exception-safe insert path

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "zeta/bits/bit_ops.h"

// ── SIMD backend selection ─────────────────────────────────────────
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
  #include <arm_neon.h>
  #define ZETA_NEON 1
#elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
  #include <emmintrin.h>
  #define ZETA_SSE2 1
#else
  #define ZETA_SCALAR 1
#endif

namespace zeta {
namespace container_internal {

// ── Constants ──────────────────────────────────────────────────────
enum Ctrl : int8_t {
    kEmpty    = static_cast<int8_t>(0b10000000),  // -128
    kDeleted  = static_cast<int8_t>(0b11111110),  // -2
    kSentinel = kDeleted,
};

constexpr size_t kGroupWidth = 16;
constexpr size_t kWidthMinus1 = 15;

// ── H2 ─────────────────────────────────────────────────────────────
inline uint8_t H2(size_t hash) noexcept {
    return static_cast<uint8_t>((hash >> 57) & 0x7F);
}

// ── SIMD bitmask extraction ────────────────────────────────────────
inline uint32_t MatchBitmask(const int8_t* ctrl, uint8_t h2) noexcept {
#if ZETA_NEON
    uint8x16_t c = vld1q_u8(reinterpret_cast<const uint8_t*>(ctrl));
    uint8x16_t match = vceqq_u8(c, vdupq_n_u8(h2));
    static const uint8_t bits[16] = {1,2,4,8,16,32,64,128,1,2,4,8,16,32,64,128};
    uint8x16_t masked = vandq_u8(match, vld1q_u8(bits));
    uint8x8_t p1 = vpadd_u8(vget_low_u8(masked), vget_high_u8(masked));
    uint8x8_t p2 = vpadd_u8(p1, p1);
    uint8x8_t p3 = vpadd_u8(p2, p2);
    return vget_lane_u16(vreinterpret_u16_u8(p3), 0);
#elif ZETA_SSE2
    __m128i ctrl_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ctrl));
    __m128i h2_vec   = _mm_set1_epi8(static_cast<char>(h2));
    return static_cast<uint32_t>(
        _mm_movemask_epi8(_mm_cmpeq_epi8(ctrl_vec, h2_vec)));
#else
    uint32_t mask = 0;
    for (size_t i = 0; i < kGroupWidth; ++i)
        if (static_cast<uint8_t>(ctrl[i]) == h2) mask |= (1u << i);
    return mask;
#endif
}

inline uint32_t MatchEmptyOrDeleted(const int8_t* ctrl) noexcept {
#if ZETA_NEON
    uint8x16_t c = vld1q_u8(reinterpret_cast<const uint8_t*>(ctrl));
    uint8x16_t m = vorrq_u8(
        vceqq_u8(c, vdupq_n_u8(static_cast<uint8_t>(kEmpty))),
        vceqq_u8(c, vdupq_n_u8(static_cast<uint8_t>(kDeleted))));
    static const uint8_t bits[16] = {1,2,4,8,16,32,64,128,1,2,4,8,16,32,64,128};
    uint8x16_t masked = vandq_u8(m, vld1q_u8(bits));
    uint8x8_t p1 = vpadd_u8(vget_low_u8(masked), vget_high_u8(masked));
    uint8x8_t p2 = vpadd_u8(p1, p1);
    uint8x8_t p3 = vpadd_u8(p2, p2);
    return vget_lane_u16(vreinterpret_u16_u8(p3), 0);
#elif ZETA_SSE2
    __m128i c = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ctrl));
    return static_cast<uint32_t>(_mm_movemask_epi8(_mm_or_si128(
        _mm_cmpeq_epi8(c, _mm_set1_epi8(static_cast<char>(kEmpty))),
        _mm_cmpeq_epi8(c, _mm_set1_epi8(static_cast<char>(kDeleted))))));
#else
    uint32_t mask = 0;
    for (size_t i = 0; i < kGroupWidth; ++i)
        if (ctrl[i] == kEmpty || ctrl[i] == kDeleted) mask |= (1u << i);
    return mask;
#endif
}

inline uint32_t MatchEmpty(const int8_t* ctrl) noexcept {
#if ZETA_NEON
    uint8x16_t c = vld1q_u8(reinterpret_cast<const uint8_t*>(ctrl));
    uint8x16_t m = vceqq_u8(c, vdupq_n_u8(static_cast<uint8_t>(kEmpty)));
    static const uint8_t bits[16] = {1,2,4,8,16,32,64,128,1,2,4,8,16,32,64,128};
    uint8x16_t masked = vandq_u8(m, vld1q_u8(bits));
    uint8x8_t p1 = vpadd_u8(vget_low_u8(masked), vget_high_u8(masked));
    uint8x8_t p2 = vpadd_u8(p1, p1);
    uint8x8_t p3 = vpadd_u8(p2, p2);
    return vget_lane_u16(vreinterpret_u16_u8(p3), 0);
#elif ZETA_SSE2
    __m128i c = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ctrl));
    return static_cast<uint32_t>(_mm_movemask_epi8(
        _mm_cmpeq_epi8(c, _mm_set1_epi8(static_cast<char>(kEmpty)))));
#else
    uint32_t mask = 0;
    for (size_t i = 0; i < kGroupWidth; ++i)
        if (ctrl[i] == kEmpty) mask |= (1u << i);
    return mask;
#endif
}

inline size_t LowestBitSet(uint32_t mask) noexcept {
    return static_cast<size_t>(zeta::countr_zero(mask));
}

// ── Policy helpers ─────────────────────────────────────────────────
template <typename K, typename V>
struct MapPolicy {
    using key_type   = K;
    using value_type = std::pair<const K, V>;
    static const key_type& get_key(const value_type& v) noexcept { return v.first; }
};

template <typename K>
struct SetPolicy {
    using key_type   = K;
    using value_type = K;
    static const key_type& get_key(const value_type& v) noexcept { return v; }
};

// ═══════════════════════════════════════════════════════════════════
// raw_hash_set
// ═══════════════════════════════════════════════════════════════════
template <typename Policy, typename Hash, typename KeyEq>
class raw_hash_set {
public:
    using key_type        = typename Policy::key_type;
    using stored_value_type = typename Policy::value_type;
    using hasher          = Hash;
    using key_equal       = KeyEq;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

private:
    // ── iterator_impl (templated on const-ness) ───────────────────
    template <bool IsConst>
    class iterator_impl {
        using maybe_const = std::conditional_t<IsConst, const stored_value_type,
                                                       stored_value_type>;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = stored_value_type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = maybe_const*;
        using reference         = maybe_const&;

        iterator_impl() noexcept
            : slot_(nullptr), ctrl_(nullptr), end_slot_(nullptr) {}

        // implicit conversion: iterator → const_iterator
        template <bool OtherConst,
                  typename = std::enable_if_t<IsConst && !OtherConst>>
        iterator_impl(const iterator_impl<OtherConst>& other) noexcept
            : slot_(other.slot_), ctrl_(other.ctrl_),
              end_slot_(other.end_slot_) {}

        reference operator*()  const noexcept { return *slot_; }
        pointer   operator->() const noexcept { return slot_; }

        iterator_impl& operator++() noexcept {
            ++slot_; ++ctrl_;
            skip_empty_or_deleted();
            return *this;
        }
        iterator_impl operator++(int) noexcept {
            iterator_impl tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const iterator_impl& a,
                               const iterator_impl& b) noexcept {
            return a.slot_ == b.slot_;
        }
        friend bool operator!=(const iterator_impl& a,
                               const iterator_impl& b) noexcept {
            return !(a == b);
        }

    private:
        friend class raw_hash_set;
        template <bool> friend class iterator_impl;

        maybe_const*  slot_;
        const int8_t* ctrl_;
        maybe_const*  end_slot_;

        // Normal constructor: walks to first valid element
        iterator_impl(maybe_const* slot, const int8_t* ctrl,
                       maybe_const* end_slot) noexcept
            : slot_(slot), ctrl_(ctrl), end_slot_(end_slot) {
            skip_empty_or_deleted();
        }

        // End sentinel: slot_ == end_slot_ so skip is a no-op
        struct end_tag_t {};
        iterator_impl(end_tag_t, maybe_const* end_slot) noexcept
            : slot_(end_slot), ctrl_(nullptr), end_slot_(end_slot) {}

        void skip_empty_or_deleted() noexcept {
            while (slot_ != end_slot_ &&
                   (*ctrl_ == kEmpty || *ctrl_ == kDeleted)) {
                ++slot_; ++ctrl_;
            }
        }
    };

public:
    using iterator       = iterator_impl<false>;
    using const_iterator = iterator_impl<true>;
    using reference      = stored_value_type&;
    using const_reference = const stored_value_type&;
    using pointer        = stored_value_type*;
    using const_pointer  = const stored_value_type*;

    // ── Construction / destruction ────────────────────────────────
    raw_hash_set() noexcept { reset_layout(); }

    explicit raw_hash_set(size_t bucket_count,
                          const Hash& hash = Hash(),
                          const KeyEq& eq = KeyEq())
        : hash_(hash), eq_(eq) {
        reset_layout();
        if (bucket_count > 0) rehash(bucket_count);
    }

    ~raw_hash_set() noexcept { destroy_slots(); }

    raw_hash_set(const raw_hash_set& other)
        : hash_(other.hash_), eq_(other.eq_) {
        reset_layout();
        copy_from(other);
    }
    raw_hash_set& operator=(const raw_hash_set& other) {
        if (this != &other) {
            hash_ = other.hash_; eq_ = other.eq_;
            destroy_slots();
            reset_layout();
            copy_from(other);
        }
        return *this;
    }

    raw_hash_set(raw_hash_set&& other) noexcept
        : ctrl_(other.ctrl_), slots_(other.slots_),
          size_(other.size_), capacity_(other.capacity_),
          hash_(std::move(other.hash_)), eq_(std::move(other.eq_)) {
        other.reset_layout();
    }
    raw_hash_set& operator=(raw_hash_set&& other) noexcept {
        if (this != &other) {
            destroy_slots();
            ctrl_ = other.ctrl_;  slots_ = other.slots_;
            size_ = other.size_;  capacity_ = other.capacity_;
            hash_ = std::move(other.hash_);
            eq_   = std::move(other.eq_);
            other.reset_layout();
        }
        return *this;
    }

    // ── Capacity ──────────────────────────────────────────────────
    size_t size()      const noexcept { return size_; }
    size_t capacity()  const noexcept { return capacity_; }
    bool   empty()     const noexcept { return size_ == 0; }
    size_t max_size()  const noexcept {
        // Capacity is always a power of 2, and the load factor is 7/8.
        // The maximum representable power of 2 is SIZE_MAX/2 + 1, which
        // limits the maximum number of elements to ~87.5% of that.
        return (std::numeric_limits<size_t>::max() >> 3) * 7;
    }

    // ── Public rehash / reserve ───────────────────────────────────
    void reserve(size_t n) { if (n > capacity_) rehash(n); }
    void rehash(size_t n)  { rehash_impl(n); }

    // ── Lookup (heterogeneous when Hash/KeyEq are transparent) ────
    template <typename K = key_type>
    iterator find(const K& key) {
        size_t idx = find_impl(key, hash_(key));
        return iterator_at(idx);
    }
    template <typename K = key_type>
    const_iterator find(const K& key) const {
        size_t idx = find_impl(key, hash_(key));
        return const_iterator_at(idx);
    }

    template <typename K = key_type>
    bool contains(const K& key) const {
        return find_impl(key, hash_(key)) != capacity_;
    }

    template <typename K = key_type>
    size_t count(const K& key) const {
        return contains(key) ? 1 : 0;
    }

    // ── Insert ────────────────────────────────────────────────────
    std::pair<iterator, bool> insert(const stored_value_type& v) {
        return emplace(v);
    }
    std::pair<iterator, bool> insert(stored_value_type&& v) {
        return emplace(std::move(v));
    }

    /// try_emplace: only construct value if key not present.
    /// Receives key separately from value constructor args.
    template <typename... Args>
    std::pair<iterator, bool> try_emplace(const key_type& key,
                                          Args&&... args) {
        size_t hash_val = hash_(key);
        size_t idx = find_impl(key, hash_val);
        if (idx != capacity_) return {iterator_at(idx), false};
        stored_value_type v(std::piecewise_construct,
                     std::forward_as_tuple(key),
                     std::forward_as_tuple(std::forward<Args>(args)...));
        return emplace_new(key, hash_val, std::move(v));
    }
    template <typename... Args>
    std::pair<iterator, bool> try_emplace(key_type&& key,
                                          Args&&... args) {
        size_t hash_val = hash_(key);
        size_t idx = find_impl(key, hash_val);
        if (idx != capacity_) return {iterator_at(idx), false};
        stored_value_type v(std::piecewise_construct,
                     std::forward_as_tuple(std::move(key)),
                     std::forward_as_tuple(std::forward<Args>(args)...));
        return emplace_new(v.first, hash_val, std::move(v));
    }

    /// insert_or_assign: overwrite if key exists, insert otherwise.
    template <typename M>
    std::pair<iterator, bool> insert_or_assign(const key_type& key,
                                                M&& obj) {
        size_t hash_val = hash_(key);
        size_t idx = find_impl(key, hash_val);
        if (idx != capacity_) {
            slots_[idx].second = std::forward<M>(obj);
            return {iterator_at(idx), false};
        }
        stored_value_type v(key, std::forward<M>(obj));
        return emplace_new(key, hash_val, std::move(v));
    }
    template <typename M>
    std::pair<iterator, bool> insert_or_assign(key_type&& key,
                                                M&& obj) {
        size_t hash_val = hash_(key);
        size_t idx = find_impl(key, hash_val);
        if (idx != capacity_) {
            slots_[idx].second = std::forward<M>(obj);
            return {iterator_at(idx), false};
        }
        stored_value_type v(std::move(key), std::forward<M>(obj));
        const key_type& k = Policy::get_key(v);
        return emplace_new(k, hash_val, std::move(v));
    }

    // ── Emplace (general) ─────────────────────────────────────────
    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        return emplace_impl(std::forward<Args>(args)...);
    }

    // ── Erase (heterogeneous) ─────────────────────────────────────
    iterator erase(const_iterator pos) {
        size_t idx = static_cast<size_t>(pos.slot_ - slots_);
        pos.slot_->~stored_value_type();
        set_ctrl(idx, kDeleted);
        --size_;
        return iterator_at(idx + 1); // next valid
    }

    // SFINAE guard: K must not be iterator or const_iterator
    template <typename K = key_type,
              typename = std::enable_if_t<
                  !std::is_convertible_v<const K&, const_iterator>>>
    size_t erase(const K& key) {
        size_t idx = find_impl(key, hash_(key));
        if (idx == capacity_) return 0;
        slots_[idx].~stored_value_type();
        set_ctrl(idx, kDeleted);
        --size_;
        return 1;
    }

    void clear() {
        destroy_slots();
        reset_layout();
    }

    // ── Iterators ─────────────────────────────────────────────────
    iterator       begin()        noexcept { return make_iter(slots_); }
    iterator       end()          noexcept {
        return capacity_ ? iterator{iter_end_tag{}, slots_ + capacity_}
                         : iterator{};
    }
    const_iterator begin()  const noexcept { return make_citer(slots_); }
    const_iterator end()    const noexcept {
        return capacity_ ? const_iterator{citer_end_tag{}, slots_ + capacity_}
                         : const_iterator{};
    }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend()   const noexcept { return end(); }

    // ── operator[] (map only) ─────────────────────────────────────
    template <typename V = stored_value_type>
    typename V::second_type& operator[](const key_type& key) {
        size_t hash_val = hash_(key);
        size_t idx = find_impl(key, hash_val);
        if (idx != capacity_) return slots_[idx].second;
        stored_value_type v(key, typename V::second_type{});
        auto [it, _] = emplace_new(key, hash_val, std::move(v));
        return it->second;
    }
    template <typename V = stored_value_type>
    typename V::second_type& operator[](key_type&& key) {
        size_t hash_val = hash_(key);
        size_t idx = find_impl(key, hash_val);
        if (idx != capacity_) return slots_[idx].second;
        stored_value_type v(std::move(key), typename V::second_type{});
        auto [it, _] = emplace_new(v.first, hash_val, std::move(v));
        return it->second;
    }

    // ── Swap ──────────────────────────────────────────────────────
    void swap(raw_hash_set& other) noexcept {
        using std::swap;
        swap(ctrl_,     other.ctrl_);
        swap(slots_,    other.slots_);
        swap(size_,     other.size_);
        swap(capacity_, other.capacity_);
        swap(hash_,     other.hash_);
        swap(eq_,       other.eq_);
    }
    friend void swap(raw_hash_set& a, raw_hash_set& b) noexcept { a.swap(b); }

    // ── Equality ──────────────────────────────────────────────────
    bool operator==(const raw_hash_set& other) const {
        if (size_ != other.size_) return false;
        for (const auto& v : other) {
            auto it = find(Policy::get_key(v));
            if (it == end() || !eq_(Policy::get_key(v),
                                    Policy::get_key(*it)))
                return false;
        }
        return true;
    }
    bool operator!=(const raw_hash_set& other) const {
        return !(*this == other);
    }

private:
    int8_t*     ctrl_     = nullptr;
    stored_value_type* slots_    = nullptr;
    size_t      size_     = 0;
    size_t      capacity_ = 0;
    [[no_unique_address]] Hash   hash_;
    [[no_unique_address]] KeyEq  eq_;

    // ── Internal iterator factories ───────────────────────────────
    using iter_end_tag = typename iterator::end_tag_t;
    using citer_end_tag = typename const_iterator::end_tag_t;

    iterator make_iter(stored_value_type* slot) noexcept {
        if (capacity_ == 0) return end();
        return iterator{slot, ctrl_ + (slot - slots_),
                        slots_ + capacity_};
    }
    const_iterator make_citer(const stored_value_type* slot) const noexcept {
        if (capacity_ == 0) return end();
        return const_iterator{slot, ctrl_ + (slot - slots_),
                              slots_ + capacity_};
    }
    iterator iterator_at(size_t idx) noexcept {
        if (idx >= capacity_) return end();
        return iterator{slots_ + idx, ctrl_ + idx, slots_ + capacity_};
    }
    const_iterator const_iterator_at(size_t idx) const noexcept {
        if (idx >= capacity_) return end();
        return const_iterator{slots_ + idx, ctrl_ + idx,
                              slots_ + capacity_};
    }

    // ── Helpers ──────────────────────────────────────────────────
    void reset_layout() noexcept {
        ctrl_ = nullptr; slots_ = nullptr;
        size_ = 0; capacity_ = 0;
    }

    void set_ctrl(size_t i, int8_t val) noexcept {
        assert(capacity_ > 0 && i < capacity_);
        ctrl_[i] = val;
        if (i < kGroupWidth + 1) ctrl_[capacity_ + i] = val;
    }

    void destroy_slots() noexcept {
        if (!slots_) return;
        for (size_t i = 0; i < capacity_; ++i)
            if (ctrl_[i] != kEmpty && ctrl_[i] != kDeleted)
                slots_[i].~stored_value_type();
        ::operator delete(slots_);
        ::operator delete(ctrl_);
        slots_ = nullptr; ctrl_ = nullptr;
    }

    static size_t next_power_of_2(size_t n) noexcept {
        return bit_ceil(n);
    }

    // ── Rehash ────────────────────────────────────────────────────
    void rehash_impl(size_t new_capacity) {
        new_capacity = next_power_of_2(new_capacity);
        if (new_capacity <= capacity_) return;

        int8_t* old_ctrl = ctrl_;
        stored_value_type* old_slots = slots_;
        size_t old_capacity = capacity_;
        size_t old_size = size_;

        auto delete_ctrl = [](int8_t* p) noexcept { ::operator delete(p); };
        auto delete_slots = [](stored_value_type* p) noexcept { ::operator delete(p); };

        std::unique_ptr<int8_t, decltype(delete_ctrl)> new_ctrl_guard(
            static_cast<int8_t*>(::operator new(sizeof(int8_t) *
                                                (new_capacity + kGroupWidth + 1))),
            delete_ctrl);
        std::unique_ptr<stored_value_type, decltype(delete_slots)> new_slots_guard(
            static_cast<stored_value_type*>(
                ::operator new(sizeof(stored_value_type) * new_capacity)),
            delete_slots);

        // Point the table at the new storage while we populate it, but do not
        // touch the old storage until the transfer has fully succeeded.
        ctrl_ = new_ctrl_guard.get();
        slots_ = new_slots_guard.get();
        capacity_ = new_capacity;
        size_ = 0;

        std::memset(ctrl_, kEmpty, new_capacity + kGroupWidth + 1);

        try {
            if (old_slots) {
                for (size_t i = 0; i < old_capacity; ++i) {
                    if (old_ctrl[i] != kEmpty && old_ctrl[i] != kDeleted) {
                        stored_value_type* src = old_slots + i;
                        size_t hv = hash_(Policy::get_key(*src));
                        size_t pos = prepare_insert(hv);
                        ::new (slots_ + pos) stored_value_type(std::move(*src));
                        set_ctrl(pos, static_cast<int8_t>(H2(hv)));
                        ++size_;
                    }
                }
            }
        } catch (...) {
            for (size_t j = 0; j < capacity_; ++j) {
                if (ctrl_[j] != kEmpty && ctrl_[j] != kDeleted)
                    slots_[j].~stored_value_type();
            }
            ctrl_ = old_ctrl;
            slots_ = old_slots;
            capacity_ = old_capacity;
            size_ = old_size;
            throw;
        }

        if (old_slots) {
            for (size_t i = 0; i < old_capacity; ++i) {
                if (old_ctrl[i] != kEmpty && old_ctrl[i] != kDeleted)
                    old_slots[i].~stored_value_type();
            }
            ::operator delete(old_slots);
            ::operator delete(old_ctrl);
        }

        new_ctrl_guard.release();
        new_slots_guard.release();
    }

    void copy_from(const raw_hash_set& other) {
        if (other.capacity_ == 0) return;
        rehash_impl(other.capacity_);
        for (size_t i = 0; i < other.capacity_; ++i) {
            if (other.ctrl_[i] != kEmpty && other.ctrl_[i] != kDeleted) {
                const stored_value_type& src = other.slots_[i];
                size_t hv = hash_(Policy::get_key(src));
                size_t pos = prepare_insert(hv);
                ::new (slots_ + pos) stored_value_type(src);
                set_ctrl(pos, static_cast<int8_t>(H2(hv)));
                ++size_;
            }
        }
    }

    // ── Probe: find matching key or return capacity_ ──────────────
    template <typename K>
    size_t find_impl(const K& key, size_t hash_val) const noexcept {
        if (capacity_ == 0) return capacity_;
        assert(capacity_ > 0 && zeta::has_single_bit(capacity_)); // power of 2
        uint8_t h2 = H2(hash_val);
        size_t pos = hash_val & (capacity_ - 1);

        for (;;) {
            const int8_t* group = ctrl_ + pos;
            uint32_t mask = MatchBitmask(group, h2);

            while (mask != 0) {
                size_t off = LowestBitSet(mask);
                size_t idx = (pos + off) & (capacity_ - 1);
                if (ctrl_[idx] == kEmpty) return capacity_;
                if (eq_(key, Policy::get_key(slots_[idx]))) return idx;
                mask &= (mask - 1);
            }

            if (MatchEmpty(group) != 0) return capacity_;
            pos = (pos + kGroupWidth) & (capacity_ - 1);
        }
    }

    // ── Insert: find empty/deleted slot ───────────────────────────
    size_t prepare_insert(size_t hash_val) noexcept {
        assert(capacity_ > 0 && zeta::has_single_bit(capacity_));
        size_t pos = hash_val & (capacity_ - 1);
        size_t first_deleted = capacity_;

        for (;;) {
            const int8_t* group = ctrl_ + pos;
            uint32_t mask = MatchEmptyOrDeleted(group);

            while (mask != 0) {
                size_t off = LowestBitSet(mask);
                size_t idx = (pos + off) & (capacity_ - 1);
                if (ctrl_[idx] == kEmpty)
                    return (first_deleted != capacity_) ? first_deleted : idx;
                if (first_deleted == capacity_) first_deleted = idx;
                mask &= (mask - 1);
            }
            pos = (pos + kGroupWidth) & (capacity_ - 1);
        }
    }

    // ── Emplace helpers ───────────────────────────────────────────
    // General emplace (constructs value first, may be wasteful)
    template <typename... Args>
    std::pair<iterator, bool> emplace_impl(Args&&... args) {
        stored_value_type tmp(std::forward<Args>(args)...);
        const key_type& key = Policy::get_key(tmp);
        size_t hash_val = hash_(key);
        size_t idx = find_impl(key, hash_val);
        if (idx != capacity_) return {iterator_at(idx), false};
        return emplace_new(key, hash_val, std::move(tmp));
    }

    // Low-level emplace: key already hashed, not present, value args follow
    template <typename... Args>
    std::pair<iterator, bool> emplace_new(const key_type& /*key*/,
                                          size_t hash_val,
                                          Args&&... args) {
        if (size_ >= capacity_ * 7 / 8)
            rehash_impl(std::max<size_t>(capacity_ * 2, 16));
        size_t pos = prepare_insert(hash_val);
        ::new (slots_ + pos) stored_value_type(std::forward<Args>(args)...);
        set_ctrl(pos, static_cast<int8_t>(H2(hash_val)));
        ++size_;
        return {iterator_at(pos), true};
    }
};

} // namespace container_internal
} // namespace zeta

#endif // ZETA_CONTAINER_INTERNAL_RAW_HASH_SET_H
