function(zeta_fuzz_target name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE zeta)
    target_compile_options(${name} PRIVATE ${ZETA_FUZZING_ENGINE})
    target_link_options(${name} PRIVATE ${ZETA_FUZZING_ENGINE})
endfunction()

zeta_fuzz_target(strings_split_fuzz)
zeta_fuzz_target(strings_utils_fuzz)
