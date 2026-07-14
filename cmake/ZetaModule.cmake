include_guard(GLOBAL)

include(CMakeParseArguments)

## Register one header-only Zeta module.
##
## Module dependencies are deliberately checked at registration time. Keeping
## the dependency target available before a module is declared makes the layer
## order in zeta/CMakeLists.txt executable documentation.
function(zeta_module name)
    set(options)
    set(one_value_args)
    set(multi_value_args HEADERS DEPS)
    cmake_parse_arguments(ZM "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set(target "zeta_${name}")
    if(TARGET ${target})
        message(FATAL_ERROR "Zeta module '${name}' is registered more than once")
    endif()

    foreach(dependency IN LISTS ZM_DEPS)
        if(NOT TARGET ${dependency})
            message(FATAL_ERROR
                "Zeta module '${name}' depends on '${dependency}', "
                "which has not been registered yet")
        endif()
    endforeach()

    add_library(${target} INTERFACE)
    set_target_properties(${target} PROPERTIES EXPORT_NAME ${name})
    add_library(zeta::${name} ALIAS ${target})

    target_include_directories(${target} INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    )
    target_compile_features(${target} INTERFACE cxx_std_20)

    if(ZM_DEPS)
        target_link_libraries(${target} INTERFACE ${ZM_DEPS})
    endif()

    if(ZM_HEADERS)
        target_sources(zeta_headers PRIVATE ${ZM_HEADERS})
    endif()

    set_property(GLOBAL APPEND PROPERTY ZETA_EXPORT_TARGETS ${target})
endfunction()
