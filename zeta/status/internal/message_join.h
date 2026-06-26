#ifndef ZETA_STATUS_INTERNAL_MESSAGE_JOIN_H
#define ZETA_STATUS_INTERNAL_MESSAGE_JOIN_H

#include <string>
#include <string_view>

namespace zeta::status_internal {

[[nodiscard]] inline std::string JoinStatusMessage(
    std::string_view left, std::string_view separator,
    std::string_view right) {
    if (left.empty()) return std::string(right);
    if (right.empty()) return std::string(left);

    std::string out;
    out.reserve(left.size() + separator.size() + right.size());
    out.append(left.data(), left.size());
    out.append(separator.data(), separator.size());
    out.append(right.data(), right.size());
    return out;
}

} // namespace zeta::status_internal

#endif // ZETA_STATUS_INTERNAL_MESSAGE_JOIN_H
