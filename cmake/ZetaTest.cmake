include_guard(GLOBAL)

## Register a module-oriented Catch2 test executable.
function(zeta_test name module)
    if(NOT TARGET ${module})
        message(FATAL_ERROR
            "Zeta test '${name}' refers to missing module target '${module}'")
    endif()

    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE ${module} Catch2::Catch2WithMain)
    add_test(NAME ${name} COMMAND ${name})
endfunction()
