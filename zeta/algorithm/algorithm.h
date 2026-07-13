#ifndef ZETA_ALGORITHM_ALGORITHM_H
#define ZETA_ALGORITHM_ALGORITHM_H

/// @file   algorithm/algorithm.h
/// @brief  Container-friendly wrappers around standard algorithms.
///
/// These functions accept containers directly, including built-in arrays, so
/// callers do not need to spell out begin/end pairs. The `c_` prefix mirrors
/// the convention used by Abseil's container algorithm helpers.

#include <algorithm>
#include <functional>
#include <iterator>
#include <numeric>
#include <utility>

namespace zeta {

template <typename Container, typename T>
auto c_find(Container& container, const T& value) {
    using std::begin;
    using std::end;
    return std::find(begin(container), end(container), value);
}

template <typename Container, typename Predicate>
auto c_find_if(Container& container, Predicate predicate) {
    using std::begin;
    using std::end;
    return std::find_if(begin(container), end(container), std::move(predicate));
}

template <typename Container, typename T>
bool c_contains(const Container& container, const T& value) {
    using std::begin;
    using std::end;
    return std::find(begin(container), end(container), value) != end(container);
}

template <typename Container, typename Predicate>
bool c_any_of(const Container& container, Predicate predicate) {
    using std::begin;
    using std::end;
    return std::any_of(begin(container), end(container), std::move(predicate));
}

template <typename Container, typename Predicate>
bool c_all_of(const Container& container, Predicate predicate) {
    using std::begin;
    using std::end;
    return std::all_of(begin(container), end(container), std::move(predicate));
}

template <typename Container, typename Predicate>
bool c_none_of(const Container& container, Predicate predicate) {
    using std::begin;
    using std::end;
    return std::none_of(begin(container), end(container), std::move(predicate));
}

template <typename Container, typename T>
auto c_count(const Container& container, const T& value) {
    using std::begin;
    using std::end;
    return std::count(begin(container), end(container), value);
}

template <typename Container, typename Predicate>
auto c_count_if(const Container& container, Predicate predicate) {
    using std::begin;
    using std::end;
    return std::count_if(begin(container), end(container), std::move(predicate));
}

template <typename Container, typename OtherContainer>
bool c_equal(const Container& container, const OtherContainer& other) {
    using std::begin;
    using std::end;
    return std::equal(begin(container), end(container), begin(other), end(other));
}

template <typename Container, typename Predicate>
bool c_is_sorted(const Container& container, Predicate predicate) {
    using std::begin;
    using std::end;
    return std::is_sorted(begin(container), end(container), std::move(predicate));
}

template <typename Container>
bool c_is_sorted(const Container& container) {
    using std::begin;
    using std::end;
    return std::is_sorted(begin(container), end(container));
}

template <typename Container>
void c_sort(Container& container) {
    using std::begin;
    using std::end;
    std::sort(begin(container), end(container));
}

template <typename Container, typename Compare>
void c_sort(Container& container, Compare compare) {
    using std::begin;
    using std::end;
    std::sort(begin(container), end(container), std::move(compare));
}

template <typename Container, typename T>
auto c_lower_bound(Container& container, const T& value) {
    using std::begin;
    using std::end;
    return std::lower_bound(begin(container), end(container), value);
}

template <typename Container, typename T>
auto c_upper_bound(Container& container, const T& value) {
    using std::begin;
    using std::end;
    return std::upper_bound(begin(container), end(container), value);
}

template <typename Container, typename T>
bool c_binary_search(const Container& container, const T& value) {
    using std::begin;
    using std::end;
    return std::binary_search(begin(container), end(container), value);
}

template <typename Container, typename OutputIterator>
OutputIterator c_copy(const Container& container, OutputIterator output) {
    using std::begin;
    using std::end;
    return std::copy(begin(container), end(container), std::move(output));
}

template <typename Container, typename OutputIterator, typename Predicate>
OutputIterator c_copy_if(const Container& container, OutputIterator output,
                         Predicate predicate) {
    using std::begin;
    using std::end;
    return std::copy_if(begin(container), end(container), std::move(output),
                        std::move(predicate));
}

template <typename Container, typename Function>
Function c_for_each(Container& container, Function function) {
    using std::begin;
    using std::end;
    return std::for_each(begin(container), end(container), std::move(function));
}

template <typename Container, typename OutputIterator, typename UnaryOperation>
OutputIterator c_transform(const Container& container, OutputIterator output,
                           UnaryOperation operation) {
    using std::begin;
    using std::end;
    return std::transform(begin(container), end(container), std::move(output),
                          std::move(operation));
}

template <typename Container, typename OtherContainer, typename OutputIterator,
          typename BinaryOperation>
OutputIterator c_transform(const Container& container,
                           const OtherContainer& other, OutputIterator output,
                           BinaryOperation operation) {
    using std::begin;
    using std::end;
    return std::transform(begin(container), end(container), begin(other),
                          std::move(output), std::move(operation));
}

template <typename Container, typename T>
T c_accumulate(const Container& container, T initial) {
    using std::begin;
    using std::end;
    return std::accumulate(begin(container), end(container), std::move(initial));
}

template <typename Container, typename T, typename BinaryOperation>
T c_accumulate(const Container& container, T initial,
               BinaryOperation operation) {
    using std::begin;
    using std::end;
    return std::accumulate(begin(container), end(container), std::move(initial),
                           std::move(operation));
}

template <typename Container, typename T>
void c_iota(Container& container, T value) {
    using std::begin;
    using std::end;
    std::iota(begin(container), end(container), std::move(value));
}

} // namespace zeta

#endif // ZETA_ALGORITHM_ALGORITHM_H
