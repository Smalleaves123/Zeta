add_executable(zeta_example_quickstart quickstart.cpp)
target_link_libraries(zeta_example_quickstart PRIVATE zeta::container zeta::strings)

add_executable(zeta_example_status_time status_time.cpp)
target_link_libraries(zeta_example_status_time PRIVATE zeta::status zeta::strings zeta::time)

add_executable(zeta_example_time_control time_control.cpp)
target_link_libraries(zeta_example_time_control PRIVATE zeta::time)

add_executable(zeta_example_log_random log_random.cpp)
target_link_libraries(zeta_example_log_random PRIVATE zeta::log zeta::random)

add_executable(zeta_example_strings_pipeline strings_pipeline.cpp)
target_link_libraries(zeta_example_strings_pipeline PRIVATE zeta::strings)

add_executable(zeta_example_memory_views memory_views.cpp)
target_link_libraries(zeta_example_memory_views PRIVATE zeta::memory)

add_executable(zeta_example_ordered_index ordered_index.cpp)
target_link_libraries(zeta_example_ordered_index PRIVATE zeta::container zeta::strings zeta::time)

add_executable(zeta_example_hashing hashing.cpp)
target_link_libraries(zeta_example_hashing PRIVATE zeta::hash zeta::strings)
