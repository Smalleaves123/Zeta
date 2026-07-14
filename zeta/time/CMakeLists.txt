# time/ module — Civil time, clocks, timestamps, and durations
zeta_module(time HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/backoff.h
    ${CMAKE_CURRENT_SOURCE_DIR}/duration.h
    ${CMAKE_CURRENT_SOURCE_DIR}/clock.h
    ${CMAKE_CURRENT_SOURCE_DIR}/periodic_timer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/retry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stopwatch.h
    ${CMAKE_CURRENT_SOURCE_DIR}/timestamp.h
    ${CMAKE_CURRENT_SOURCE_DIR}/civil_time.h
    ${CMAKE_CURRENT_SOURCE_DIR}/time_zone.h
    ${CMAKE_CURRENT_SOURCE_DIR}/internal/format_internal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/internal/math_internal.h
)
