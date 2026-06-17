# ── Test helpers ─────────────────────────────────────────────────
function(zeta_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE zeta Catch2::Catch2WithMain)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# ── bits/ ─────────────────────────────────────────────────────────
add_subdirectory(bits)

# ── status/ ──────────────────────────────────────────────────────
add_subdirectory(status)

# ── memory/ ──────────────────────────────────────────────────────
add_subdirectory(memory)

# ── strings/ ─────────────────────────────────────────────────────
add_subdirectory(strings)

# ── container/ ───────────────────────────────────────────────────
add_subdirectory(container)

# ── random/ ──────────────────────────────────────────────────────
add_subdirectory(random)
