# synchronization/ module — mutexes, notifications, and queue primitives
zeta_module(synchronization HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/bounded_queue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/channel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lock_free_queue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mutex.h
    ${CMAKE_CURRENT_SOURCE_DIR}/notification.h
    ${CMAKE_CURRENT_SOURCE_DIR}/once.h
    ${CMAKE_CURRENT_SOURCE_DIR}/synchronized.h
)
