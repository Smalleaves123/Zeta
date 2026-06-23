add_executable(zeta_example_quickstart quickstart.cpp)
target_link_libraries(zeta_example_quickstart PRIVATE zeta::zeta)

add_executable(zeta_example_status_time status_time.cpp)
target_link_libraries(zeta_example_status_time PRIVATE zeta::zeta)

add_executable(zeta_example_log_random log_random.cpp)
target_link_libraries(zeta_example_log_random PRIVATE zeta::zeta)

add_executable(zeta_example_strings_pipeline strings_pipeline.cpp)
target_link_libraries(zeta_example_strings_pipeline PRIVATE zeta::zeta)

add_executable(zeta_example_memory_views memory_views.cpp)
target_link_libraries(zeta_example_memory_views PRIVATE zeta::zeta)

add_executable(zeta_example_ordered_index ordered_index.cpp)
target_link_libraries(zeta_example_ordered_index PRIVATE zeta::zeta)

add_executable(zeta_example_hashing hashing.cpp)
target_link_libraries(zeta_example_hashing PRIVATE zeta::zeta)
