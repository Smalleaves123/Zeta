# service/ module — request-scoped context for application services
zeta_module(service
    DEPS zeta_futures zeta_log zeta_time
    HEADERS
    log_adapter.h
    propagation.h
    request_context.h
    trace_context.h
)
