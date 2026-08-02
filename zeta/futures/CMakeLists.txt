# futures/ module — asynchronous value and cancellation composition
zeta_module(futures
    DEPS zeta_status
    HEADERS
    cancellation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/future.h
)
