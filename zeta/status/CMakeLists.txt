# status/ module — Status, StatusOr, Result, and propagation helpers
zeta_module(status HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/internal/message_join.h
    ${CMAKE_CURRENT_SOURCE_DIR}/status_chain.h
    ${CMAKE_CURRENT_SOURCE_DIR}/status.h
    ${CMAKE_CURRENT_SOURCE_DIR}/status_builder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/status_macros.h
    ${CMAKE_CURRENT_SOURCE_DIR}/statusor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/result.h
)
