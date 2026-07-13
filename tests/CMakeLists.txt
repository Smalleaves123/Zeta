# ── Test helpers ─────────────────────────────────────────────────
function(zeta_test name module)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE ${module} Catch2::Catch2WithMain)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# ── bits/ ─────────────────────────────────────────────────────────
add_subdirectory(bits)

# ── base/ ──────────────────────────────────────────────────────────
add_subdirectory(base)

# ── types/ ─────────────────────────────────────────────────────────
add_subdirectory(types)

# ── cleanup/ ───────────────────────────────────────────────────────
add_subdirectory(cleanup)

# ── flags/ ──────────────────────────────────────────────────────────
add_subdirectory(flags)

# ── hash/ ──────────────────────────────────────────────────────────
add_subdirectory(hash)

# ── log/ ────────────────────────────────────────────────────────────
add_subdirectory(log)

# ── meta/ ──────────────────────────────────────────────────────────
add_subdirectory(meta)

# ── metrics/ ───────────────────────────────────────────────────────
add_subdirectory(metrics)

# ── numeric/ ───────────────────────────────────────────────────────
add_subdirectory(numeric)

# ── synchronization/ ───────────────────────────────────────────────
add_subdirectory(synchronization)

# ── time/ ──────────────────────────────────────────────────────────
add_subdirectory(time)

# ── utility/ ───────────────────────────────────────────────────────
add_subdirectory(utility)

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
add_subdirectory(futures)
