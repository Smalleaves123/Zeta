#ifndef ZETA_SERVICE_TRACE_CONTEXT_H
#define ZETA_SERVICE_TRACE_CONTEXT_H

/// @file   service/trace_context.h
/// @brief  W3C Trace Context parsing and formatting.

#include "zeta/status/statusor.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace zeta {

class TraceContext {
public:
    TraceContext() = default;

    [[nodiscard]] static StatusOr<TraceContext> FromParts(
        std::string trace_id,
        std::string span_id,
        std::uint8_t trace_flags = 0) {
        TraceContext context(
            std::move(trace_id), std::move(span_id), trace_flags);
        const Status validation = context.Validate();
        if (!validation.ok()) return validation;
        return context;
    }

    [[nodiscard]] static StatusOr<TraceContext> ParseTraceParent(
        std::string_view value) {
        if (value.size() != 55 || value[2] != '-' || value[35] != '-' ||
            value[52] != '-' || value.substr(0, 2) != "00") {
            return InvalidArgumentError("invalid traceparent format");
        }

        const auto flags = ParseHexByte(value.substr(53, 2));
        if (!flags.has_value()) {
            return InvalidArgumentError("invalid traceparent flags");
        }

        return FromParts(
            std::string(value.substr(3, 32)),
            std::string(value.substr(36, 16)),
            *flags);
    }

    [[nodiscard]] Status Validate() const {
        if (!IsHexOfSize(trace_id_, 32) || IsAllZero(trace_id_)) {
            return InvalidArgumentError("trace id must be 32 hexadecimal characters");
        }
        if (!IsHexOfSize(span_id_, 16) || IsAllZero(span_id_)) {
            return InvalidArgumentError("span id must be 16 hexadecimal characters");
        }
        return OkStatus();
    }

    [[nodiscard]] bool empty() const noexcept {
        return trace_id_.empty() && span_id_.empty();
    }

    [[nodiscard]] std::string_view trace_id() const noexcept {
        return trace_id_;
    }

    [[nodiscard]] std::string_view span_id() const noexcept {
        return span_id_;
    }

    [[nodiscard]] std::uint8_t trace_flags() const noexcept {
        return trace_flags_;
    }

    [[nodiscard]] bool sampled() const noexcept {
        return (trace_flags_ & 0x01u) != 0;
    }

    [[nodiscard]] std::string ToTraceParent() const {
        if (!Validate().ok()) return {};
        static constexpr char kHex[] = "0123456789abcdef";
        std::string result = "00-" + trace_id_ + "-" + span_id_ + "-";
        result.push_back(kHex[(trace_flags_ >> 4) & 0x0Fu]);
        result.push_back(kHex[trace_flags_ & 0x0Fu]);
        return result;
    }

private:
    TraceContext(
        std::string trace_id,
        std::string span_id,
        std::uint8_t trace_flags)
        : trace_id_(NormalizeHex(std::move(trace_id)))
        , span_id_(NormalizeHex(std::move(span_id)))
        , trace_flags_(trace_flags) {}

    [[nodiscard]] static bool IsHex(char value) noexcept {
        return (value >= '0' && value <= '9') ||
               (value >= 'a' && value <= 'f') ||
               (value >= 'A' && value <= 'F');
    }

    [[nodiscard]] static bool IsHexOfSize(
        std::string_view value, std::size_t expected_size) noexcept {
        if (value.size() != expected_size) return false;
        for (const char ch : value) {
            if (!IsHex(ch)) return false;
        }
        return true;
    }

    [[nodiscard]] static bool IsAllZero(std::string_view value) noexcept {
        for (const char ch : value) {
            if (ch != '0') return false;
        }
        return true;
    }

    [[nodiscard]] static std::string NormalizeHex(std::string value) {
        for (char& ch : value) {
            if (ch >= 'A' && ch <= 'F') ch = static_cast<char>(ch - 'A' + 'a');
        }
        return value;
    }

    [[nodiscard]] static std::optional<std::uint8_t> ParseHexByte(
        std::string_view value) {
        if (value.size() != 2 || !IsHex(value[0]) || !IsHex(value[1])) {
            return std::nullopt;
        }
        const auto digit = [](char ch) -> std::uint8_t {
            if (ch >= '0' && ch <= '9') return static_cast<std::uint8_t>(ch - '0');
            if (ch >= 'a' && ch <= 'f') {
                return static_cast<std::uint8_t>(ch - 'a' + 10);
            }
            return static_cast<std::uint8_t>(ch - 'A' + 10);
        };
        return static_cast<std::uint8_t>((digit(value[0]) << 4) | digit(value[1]));
    }

    std::string trace_id_;
    std::string span_id_;
    std::uint8_t trace_flags_ = 0;
};

} // namespace zeta

#endif // ZETA_SERVICE_TRACE_CONTEXT_H
