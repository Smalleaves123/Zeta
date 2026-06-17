# strings/ module — StrCat, StrSplit, StrJoin, StrUtils
target_sources(zeta INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/str_cat.h
    ${CMAKE_CURRENT_SOURCE_DIR}/str_split.h
    ${CMAKE_CURRENT_SOURCE_DIR}/str_join.h
    ${CMAKE_CURRENT_SOURCE_DIR}/str_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/internal/alpha_num.h
)
