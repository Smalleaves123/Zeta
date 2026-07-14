# The module order is dependency order. Each layer only depends on layers
# above it; ZetaModule.cmake verifies that rule during configuration.
include(${PROJECT_SOURCE_DIR}/cmake/ZetaModule.cmake)

# ── Foundation: no Zeta module dependencies ───────────────────────
add_subdirectory(algorithm)
add_subdirectory(base)
add_subdirectory(bits)
add_subdirectory(cleanup)
add_subdirectory(meta)
add_subdirectory(numeric)
add_subdirectory(synchronization)

# ── Core values and platform abstractions ─────────────────────────
add_subdirectory(memory)
add_subdirectory(status)
add_subdirectory(time)
add_subdirectory(strings)
add_subdirectory(types)

# ── Data structures and reusable adapters ─────────────────────────
add_subdirectory(container)
add_subdirectory(crc)
add_subdirectory(hash)
add_subdirectory(random)
add_subdirectory(functional)

# ── Application-facing infrastructure ────────────────────────────
add_subdirectory(debugging)
add_subdirectory(flags)
add_subdirectory(futures)
add_subdirectory(log)
add_subdirectory(metrics)
add_subdirectory(utility)

get_property(ZETA_MODULE_TARGETS GLOBAL PROPERTY ZETA_EXPORT_TARGETS)
if(ZETA_MODULE_TARGETS)
    target_link_libraries(zeta INTERFACE ${ZETA_MODULE_TARGETS})
endif()
