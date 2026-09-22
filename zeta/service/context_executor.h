#ifndef ZETA_SERVICE_CONTEXT_EXECUTOR_H
#define ZETA_SERVICE_CONTEXT_EXECUTOR_H

/// @file   service/context_executor.h
/// @brief  Request-context propagation for executor tasks.

#include "zeta/futures/executor.h"
#include "zeta/service/request_context.h"

#include <functional>
#include <memory>

namespace zeta {

namespace service_internal {

inline const RequestContext*& CurrentRequestContextSlot() noexcept {
    static thread_local const RequestContext* current = nullptr;
    return current;
}

} // namespace service_internal

/// Installs a request context for the lifetime of the current scope.
///
/// Scopes may be nested. Destroying a scope restores the context that was
/// active before it was constructed.
class RequestContextScope final {
public:
    explicit RequestContextScope(const RequestContext& context) noexcept
        : previous_(service_internal::CurrentRequestContextSlot()) {
        service_internal::CurrentRequestContextSlot() = &context;
    }

    ~RequestContextScope() noexcept {
        service_internal::CurrentRequestContextSlot() = previous_;
    }

    RequestContextScope(const RequestContextScope&) = delete;
    RequestContextScope& operator=(const RequestContextScope&) = delete;

private:
    const RequestContext* previous_ = nullptr;
};

/// Returns the request context installed on the current thread, if any.
[[nodiscard]] inline const RequestContext* CurrentRequestContext() noexcept {
    return service_internal::CurrentRequestContextSlot();
}

/// Adds request-context propagation to an existing borrowed executor.
///
/// The wrapped executor must outlive this adapter and every continuation that
/// uses it. Each task receives a scope-local view of the copied context, and
/// the worker thread is restored when the task returns (including exceptions).
class ContextExecutor final : public Executor {
public:
    ContextExecutor(Executor& executor, RequestContext context)
        : executor_(executor)
        , context_(std::make_shared<const RequestContext>(std::move(context))) {}

    void Add(std::function<void()> task) override {
        if (!task) return;

        auto context = context_;
        executor_.Add([context = std::move(context), task = std::move(task)]() mutable {
            RequestContextScope scope(*context);
            task();
        });
    }

    ContextExecutor(const ContextExecutor&) = delete;
    ContextExecutor& operator=(const ContextExecutor&) = delete;
    ContextExecutor(ContextExecutor&&) = delete;
    ContextExecutor& operator=(ContextExecutor&&) = delete;

private:
    Executor& executor_;
    std::shared_ptr<const RequestContext> context_;
};

} // namespace zeta

#endif // ZETA_SERVICE_CONTEXT_EXECUTOR_H
