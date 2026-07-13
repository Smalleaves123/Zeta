#ifndef ZETA_LOG_FORMATTERS_H
#define ZETA_LOG_FORMATTERS_H

/// @file   log/formatters.h
/// @brief  Ready-to-use structured log formatters.

#include <sstream>
#include <string>
#include <string_view>

#include "zeta/log/internal/sink.h"

namespace zeta {

namespace log_internal {

inline void AppendJsonString(std::ostringstream& out, std::string_view value) {
    out << '"';
    for (const char ch : value) {
        switch (ch) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << ch; break;
        }
    }
    out << '"';
}

} // namespace log_internal

/// Formats records as one JSON object per line.
class JsonLogFormatter : public log_internal::LogFormatter {
public:
    std::string Format(const LogRecordView& record) noexcept override {
        std::ostringstream out;
        out << "{\"severity\":";
        log_internal::AppendJsonString(out,
                                       log_internal::SeverityName(record.severity));
        out << ",\"message\":";
        log_internal::AppendJsonString(out, record.message);
        out << ",\"file\":";
        log_internal::AppendJsonString(out, record.file ? record.file : "");
        out << ",\"line\":" << record.line;
        for (const auto& field : record.fields) {
            out << ',';
            log_internal::AppendJsonString(out, field.key);
            out << ':';
            log_internal::AppendJsonString(out, field.value);
        }
        out << "}\n";
        return out.str();
    }
};

} // namespace zeta

#endif // ZETA_LOG_FORMATTERS_H
