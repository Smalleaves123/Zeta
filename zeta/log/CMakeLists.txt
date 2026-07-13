zeta_module(log
    DEPS zeta_time
    HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/formatters.h
    ${CMAKE_CURRENT_SOURCE_DIR}/log.h
    ${CMAKE_CURRENT_SOURCE_DIR}/internal/severity.h
    ${CMAKE_CURRENT_SOURCE_DIR}/internal/sink.h
    ${CMAKE_CURRENT_SOURCE_DIR}/record.h
)
