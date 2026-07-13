# strings/ module — text construction, parsing, matching, and escaping
zeta_module(strings
    DEPS zeta_status
    HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/ascii.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cord.h
    ${CMAKE_CURRENT_SOURCE_DIR}/escaping.h
    ${CMAKE_CURRENT_SOURCE_DIR}/str_cat.h
    ${CMAKE_CURRENT_SOURCE_DIR}/match.h
    ${CMAKE_CURRENT_SOURCE_DIR}/str_split.h
    ${CMAKE_CURRENT_SOURCE_DIR}/str_join.h
    ${CMAKE_CURRENT_SOURCE_DIR}/str_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/substitute.h
    ${CMAKE_CURRENT_SOURCE_DIR}/format.h
    ${CMAKE_CURRENT_SOURCE_DIR}/numbers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/internal/alpha_num.h
)
