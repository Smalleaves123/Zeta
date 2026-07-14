# debugging/ module — assertions, stack traces, and failure handling
zeta_module(debugging
    DEPS zeta_base
    HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/assert.h
    ${CMAKE_CURRENT_SOURCE_DIR}/crash_handler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stack_trace.h
)
