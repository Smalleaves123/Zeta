#ifndef ZETA_CONTAINER_INTRUSIVE_LIST_H
#define ZETA_CONTAINER_INTRUSIVE_LIST_H

/// @file   container/intrusive_list.h
/// @brief  Non-owning doubly linked list with embedded node hooks.

#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace zeta {

class IntrusiveListHook;

template <typename T, IntrusiveListHook T::* HookMember>
class IntrusiveList;

class IntrusiveListHook {
public:
    IntrusiveListHook() noexcept = default;
    IntrusiveListHook(const IntrusiveListHook&) = delete;
    IntrusiveListHook& operator=(const IntrusiveListHook&) = delete;
    IntrusiveListHook(IntrusiveListHook&&) = delete;
    IntrusiveListHook& operator=(IntrusiveListHook&&) = delete;

    [[nodiscard]] bool IsLinked() const noexcept {
        return list_ != nullptr;
    }

private:
    IntrusiveListHook* previous_ = nullptr;
    IntrusiveListHook* next_ = nullptr;
    void* object_ = nullptr;
    void* list_ = nullptr;

    template <typename T, IntrusiveListHook T::* HookMember>
    friend class IntrusiveList;
};

template <typename T, IntrusiveListHook T::* HookMember>
class IntrusiveList {
    using Hook = IntrusiveListHook;

    template <bool IsConst>
    class BasicIterator {
        using HookPointer = std::conditional_t<IsConst, const Hook*, Hook*>;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<IsConst, const T*, T*>;
        using reference = std::conditional_t<IsConst, const T&, T&>;

        BasicIterator() noexcept = default;

        template <bool OtherConst>
        requires IsConst && (!OtherConst)
        BasicIterator(const BasicIterator<OtherConst>& other) noexcept
            : hook_(other.hook_) {}

        reference operator*() const noexcept {
            return Value(*hook_);
        }

        pointer operator->() const noexcept {
            return &Value(*hook_);
        }

        BasicIterator& operator++() noexcept {
            hook_ = hook_->next_;
            return *this;
        }

        BasicIterator operator++(int) noexcept {
            BasicIterator copy = *this;
            ++*this;
            return copy;
        }

        BasicIterator& operator--() noexcept {
            hook_ = hook_->previous_;
            return *this;
        }

        BasicIterator operator--(int) noexcept {
            BasicIterator copy = *this;
            --*this;
            return copy;
        }

        friend bool operator==(
            const BasicIterator& left, const BasicIterator& right) noexcept {
            return left.hook_ == right.hook_;
        }

    private:
        explicit BasicIterator(HookPointer hook) noexcept : hook_(hook) {}

        static reference Value(const Hook& hook) noexcept {
            return *static_cast<pointer>(hook.object_);
        }

        HookPointer hook_ = nullptr;
        friend class IntrusiveList;
        template <bool>
        friend class BasicIterator;
    };

public:
    using value_type = T;
    using size_type = std::size_t;
    using iterator = BasicIterator<false>;
    using const_iterator = BasicIterator<true>;

    IntrusiveList() noexcept {
        root_.previous_ = &root_;
        root_.next_ = &root_;
    }

    IntrusiveList(const IntrusiveList&) = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;
    IntrusiveList(IntrusiveList&&) = delete;
    IntrusiveList& operator=(IntrusiveList&&) = delete;
    ~IntrusiveList() = default;

    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] size_type size() const noexcept { return size_; }

    iterator begin() noexcept { return iterator(root_.next_); }
    const_iterator begin() const noexcept { return const_iterator(root_.next_); }
    const_iterator cbegin() const noexcept { return begin(); }
    iterator end() noexcept { return iterator(&root_); }
    const_iterator end() const noexcept { return const_iterator(&root_); }
    const_iterator cend() const noexcept { return end(); }

    T& front() noexcept {
        assert(!empty());
        return Value(*root_.next_);
    }

    const T& front() const noexcept {
        assert(!empty());
        return Value(*root_.next_);
    }

    T& back() noexcept {
        assert(!empty());
        return Value(*root_.previous_);
    }

    const T& back() const noexcept {
        assert(!empty());
        return Value(*root_.previous_);
    }

    bool push_front(T& value) noexcept {
        return InsertBefore(root_.next_, value);
    }

    bool push_back(T& value) noexcept {
        return InsertBefore(&root_, value);
    }

    bool erase(T& value) noexcept {
        Hook& hook = value.*HookMember;
        if (hook.list_ != this) return false;
        Unlink(hook);
        return true;
    }

    iterator erase(iterator position) noexcept {
        if (position.hook_ == &root_ || position.hook_->list_ != this) {
            return end();
        }
        Hook* next = position.hook_->next_;
        Unlink(*position.hook_);
        return iterator(next);
    }

    void clear() noexcept {
        while (!empty()) erase(begin());
    }

private:
    static Hook& HookOf(T& value) noexcept { return value.*HookMember; }

    static T& Value(Hook& hook) noexcept {
        return *static_cast<T*>(hook.object_);
    }

    static const T& Value(const Hook& hook) noexcept {
        return *static_cast<const T*>(hook.object_);
    }

    bool InsertBefore(Hook* position, T& value) noexcept {
        Hook& hook = HookOf(value);
        if (hook.IsLinked()) return false;

        hook.object_ = &value;
        hook.list_ = this;
        hook.previous_ = position->previous_;
        hook.next_ = position;
        position->previous_->next_ = &hook;
        position->previous_ = &hook;
        ++size_;
        return true;
    }

    void Unlink(Hook& hook) noexcept {
        hook.previous_->next_ = hook.next_;
        hook.next_->previous_ = hook.previous_;
        hook.previous_ = nullptr;
        hook.next_ = nullptr;
        hook.object_ = nullptr;
        hook.list_ = nullptr;
        --size_;
    }

    Hook root_;
    size_type size_ = 0;
};

} // namespace zeta

#endif // ZETA_CONTAINER_INTRUSIVE_LIST_H
