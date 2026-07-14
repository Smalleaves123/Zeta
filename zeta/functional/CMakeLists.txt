# functional/ module — callable composition helpers
zeta_module(functional
    DEPS zeta_memory
    HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/compose.h
    ${CMAKE_CURRENT_SOURCE_DIR}/overload.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pipe.h
)
