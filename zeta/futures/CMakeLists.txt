# futures/ module — asynchronous value and cancellation composition
zeta_module(futures
    DEPS zeta_status
    HEADERS
    async_queue.h
    cancellation.h
    executor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/future.h
    ${CMAKE_CURRENT_SOURCE_DIR}/task_group.h
)
target_link_libraries(zeta_futures INTERFACE Threads::Threads)
