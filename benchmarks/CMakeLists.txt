function(zeta_benchmark name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE zeta benchmark::benchmark)
endfunction()

zeta_benchmark(flat_hash_map_bench)
