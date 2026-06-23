#ifndef ZETA_STRINGS_CORD_H
#define ZETA_STRINGS_CORD_H

/// @file   strings/cord.h
/// @brief  Chunked string container for append-heavy workloads.
///
/// `zeta::Cord` stores text as a sequence of owned chunks.  Appending a
/// `std::string` can move the payload into the cord without flattening the
/// whole value, while `Flatten()` produces a contiguous `std::string` when
/// needed.

#include <cstddef>
#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace zeta {

class Cord {
public:
    Cord() = default;

    Cord(std::string_view text) { Append(text); }  // NOLINT
    Cord(const char* text) { Append(text); }       // NOLINT
    Cord(std::string text) { Append(std::move(text)); } // NOLINT

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] std::size_t chunk_count() const noexcept {
        return chunks_.size();
    }

    void clear() noexcept {
        chunks_.clear();
        size_ = 0;
    }

    void RemovePrefix(std::size_t n) {
        if (n == 0 || empty()) return;
        if (n >= size_) {
            clear();
            return;
        }

        size_ -= n;
        while (n > 0 && !chunks_.empty()) {
            if (n >= chunks_.front().size()) {
                n -= chunks_.front().size();
                chunks_.erase(chunks_.begin());
            } else {
                chunks_.front().erase(0, n);
                n = 0;
            }
        }
    }

    void RemoveSuffix(std::size_t n) {
        if (n == 0 || empty()) return;
        if (n >= size_) {
            clear();
            return;
        }

        size_ -= n;
        while (n > 0 && !chunks_.empty()) {
            if (n >= chunks_.back().size()) {
                n -= chunks_.back().size();
                chunks_.pop_back();
            } else {
                chunks_.back().resize(chunks_.back().size() - n);
                n = 0;
            }
        }
    }

    [[nodiscard]] Cord Subcord(std::size_t pos,
                               std::size_t len = static_cast<std::size_t>(-1)) const {
        Cord out;
        if (pos >= size_ || len == 0) return out;

        std::size_t remaining = std::min(len, size_ - pos);
        std::size_t offset = pos;
        for (const auto& chunk : chunks_) {
            if (remaining == 0) break;
            if (offset >= chunk.size()) {
                offset -= chunk.size();
                continue;
            }

            std::size_t take = std::min(remaining, chunk.size() - offset);
            out.Append(std::string_view(chunk).substr(offset, take));
            remaining -= take;
            offset = 0;
        }
        return out;
    }

    [[nodiscard]] bool StartsWith(std::string_view prefix) const {
        if (prefix.size() > size_) return false;
        std::size_t matched = 0;
        for (const auto& chunk : chunks_) {
            if (matched == prefix.size()) return true;
            std::size_t take = std::min(prefix.size() - matched, chunk.size());
            if (std::string_view(chunk).substr(0, take) !=
                prefix.substr(matched, take)) {
                return false;
            }
            matched += take;
        }
        return matched == prefix.size();
    }

    [[nodiscard]] bool EndsWith(std::string_view suffix) const {
        if (suffix.size() > size_) return false;
        return Subcord(size_ - suffix.size()).Flatten() == suffix;
    }

    void Append(std::string_view text) {
        if (text.empty()) return;
        chunks_.emplace_back(text);
        size_ += text.size();
    }

    void Append(const char* text) {
        if (text == nullptr || *text == '\0') return;
        Append(std::string_view(text));
    }

    void Append(char c) {
        chunks_.emplace_back(1, c);
        ++size_;
    }

    void Append(std::string text) {
        if (text.empty()) return;
        size_ += text.size();
        chunks_.push_back(std::move(text));
    }

    void Append(const Cord& other) {
        if (other.empty()) return;
        chunks_.reserve(chunks_.size() + other.chunks_.size());
        chunks_.insert(chunks_.end(), other.chunks_.begin(), other.chunks_.end());
        size_ += other.size_;
    }

    void Append(Cord&& other) {
        if (other.empty()) return;
        chunks_.reserve(chunks_.size() + other.chunks_.size());
        for (auto& chunk : other.chunks_) {
            chunks_.push_back(std::move(chunk));
        }
        size_ += other.size_;
        other.clear();
    }

    [[nodiscard]] std::string Flatten() const {
        std::string out;
        out.reserve(size_);
        AppendTo(&out);
        return out;
    }

    void AppendTo(std::string* out) const {
        if (out == nullptr || empty()) return;
        out->reserve(out->size() + size_);
        for (const auto& chunk : chunks_) {
            out->append(chunk);
        }
    }

    template <typename Fn>
    void ForEachChunk(Fn&& fn) const {
        for (const auto& chunk : chunks_) {
            fn(std::string_view(chunk));
        }
    }

    Cord& operator+=(std::string_view text) {
        Append(text);
        return *this;
    }

    Cord& operator+=(const char* text) {
        Append(text);
        return *this;
    }

    Cord& operator+=(char c) {
        Append(c);
        return *this;
    }

    Cord& operator+=(const Cord& other) {
        Append(other);
        return *this;
    }

    friend bool operator==(const Cord& a, const Cord& b) {
        return a.size_ == b.size_ && a.Flatten() == b.Flatten();
    }

    friend bool operator!=(const Cord& a, const Cord& b) {
        return !(a == b);
    }

private:
    std::vector<std::string> chunks_;
    std::size_t size_ = 0;
};

} // namespace zeta

#endif // ZETA_STRINGS_CORD_H
