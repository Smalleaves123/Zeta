# types/ module — optional-style value utilities
zeta_module(types HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/any.h
    ${CMAKE_CURRENT_SOURCE_DIR}/optional.h
    ${CMAKE_CURRENT_SOURCE_DIR}/variant.h
    DEPS zeta_base
)
