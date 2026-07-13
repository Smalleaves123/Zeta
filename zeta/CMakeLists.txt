include(CMakeParseArguments)

function(zeta_module name)
    set(options)
    set(one_value_args)
    set(multi_value_args HEADERS DEPS)
    cmake_parse_arguments(ZM "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set(target "zeta_${name}")
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

# ── Per-module CMake (Abseil-style interface targets) ─────────────
add_subdirectory(bits)
add_subdirectory(base)
add_subdirectory(cleanup)
add_subdirectory(flags)
add_subdirectory(hash)
add_subdirectory(log)
add_subdirectory(memory)
add_subdirectory(meta)
add_subdirectory(numeric)
add_subdirectory(random)
add_subdirectory(futures)
add_subdirectory(status)
add_subdirectory(types)
add_subdirectory(strings)
add_subdirectory(synchronization)
add_subdirectory(time)
add_subdirectory(metrics)
add_subdirectory(utility)
add_subdirectory(container)

get_property(ZETA_MODULE_TARGETS GLOBAL PROPERTY ZETA_EXPORT_TARGETS)
if(ZETA_MODULE_TARGETS)
    target_link_libraries(zeta INTERFACE ${ZETA_MODULE_TARGETS})
endif()
