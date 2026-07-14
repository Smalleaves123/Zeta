#ifndef ZETA_STATUS_STATUS_CHAIN_H
#define ZETA_STATUS_STATUS_CHAIN_H

/// @file   status/status_chain.h
/// @brief  Composable error context and cause-chain support.

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "zeta/status/status.h"

namespace zeta {

/// A status plus structured context and underlying causes.
///
/// `Status` remains a compact value type. Use `StatusChain` at service or
/// request boundaries when preserving the failure path is more useful than
/// storing one flattened message.
class StatusChain {
public:
    explicit StatusChain(Status status) : status_(std::move(status)) {}

    [[nodiscard]] const Status& status() const noexcept { return status_; }
    [[nodiscard]] const std::vector<Status>& causes() const noexcept {
        return causes_;
    }

    [[nodiscard]] bool ok() const noexcept { return status_.ok(); }

    [[nodiscard]] bool ContainsCode(StatusCode code) const noexcept {
        if (status_.code() == code) return true;
        for (const auto& cause : causes_) {
            if (cause.code() == code) return true;
        }
        return false;
    }

    StatusChain& AddContext(std::string_view context,
                            std::string_view separator = ": ") {
        if (status_.ok() || context.empty()) return *this;
        std::string message(status_.message());
        if (!message.empty()) message.append(separator);
        message.append(context);
        status_ = Status(status_.code(), std::move(message));
        return *this;
    }

    StatusChain& CausedBy(Status cause) {
        if (!cause.ok()) causes_.push_back(std::move(cause));
        return *this;
    }

    StatusChain& CausedBy(const StatusChain& cause) {
        if (!cause.status_.ok()) causes_.push_back(cause.ToStatus());
        causes_.insert(causes_.end(), cause.causes_.begin(), cause.causes_.end());
        return *this;
    }

    /// Flattens the chain into a normal Status for existing StatusOr APIs.
    [[nodiscard]] Status ToStatus() const {
        if (status_.ok() || causes_.empty()) return status_;

        std::string message(status_.message());
        for (const auto& cause : causes_) {
            if (!message.empty()) message.append("; ");
            message.append("caused by ");
            message.append(cause.ToString());
        }
        return Status(status_.code(), std::move(message));
    }

    [[nodiscard]] std::string ToString() const { return ToStatus().ToString(); }

private:
    Status status_;
    std::vector<Status> causes_;
};

[[nodiscard]] inline StatusChain ChainStatus(Status status) {
    return StatusChain(std::move(status));
}

} // namespace zeta

#endif // ZETA_STATUS_STATUS_CHAIN_H
