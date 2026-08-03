# container/ module — maps, caches, lists, and inline-storage containers
zeta_module(container
    DEPS zeta_bits
    HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/btree_map.h
    ${CMAKE_CURRENT_SOURCE_DIR}/btree_set.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fixed_array.h
    ${CMAKE_CURRENT_SOURCE_DIR}/flat_hash_map.h
    ${CMAKE_CURRENT_SOURCE_DIR}/flat_hash_set.h
    ${CMAKE_CURRENT_SOURCE_DIR}/inlined_vector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/intrusive_list.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lfu_cache.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lru_cache.h
    ${CMAKE_CURRENT_SOURCE_DIR}/node_hash_map.h
    ${CMAKE_CURRENT_SOURCE_DIR}/node_hash_set.h
    ${CMAKE_CURRENT_SOURCE_DIR}/small_map.h
    ${CMAKE_CURRENT_SOURCE_DIR}/internal/btree_impl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/internal/raw_hash_set.h
)
