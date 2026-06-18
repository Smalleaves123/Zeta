#ifndef ZETA_FLAGS_PARSE_H
#define ZETA_FLAGS_PARSE_H

/// @file   flags/parse.h
/// @brief  Command-line parser for registered flags.
///
/// `zeta::ParseCommandLine(argc, argv)` scans `argv` for `--flag=value`
/// and `--flag value` pairs, sets the corresponding `Flag<T>`, and
/// removes them from `argv` (so non-flag arguments remain accessible).
///
/// Example:
///   int main(int argc, char** argv) {
///       zeta::ParseCommandLine(argc, argv);
///       // ... use FLAGS_xxx ...
///   }

#include <cstdio>
#include <cstdlib>
#include <string_view>

#include "zeta/flags/flag.h"
#include "zeta/flags/internal/registry.h"

namespace zeta {

// ═══════════════════════════════════════════════════════════════════════
// ParseCommandLine
// ═══════════════════════════════════════════════════════════════════════

/// Parse and consume recognised flags from `argv`.  After the call,
/// `argc` and `argv` reflect only the non-flag positional arguments.
///
/// Supports:
///   --flag=value
///   --flag value
///   --flag        (bool flags: implicitly sets to true)
///   --noflag      (bool flags: implicitly sets to false)
///   --help        (prints all registered flags and exits)
inline void ParseCommandLine(int& argc, char**& argv) noexcept {
    int write = 1;  // keep argv[0]

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);

        // ── --help ──────────────────────────────────────────────────
        if (arg == "--help" || arg == "-h") {
            std::printf("Flags:\n");
            for (auto* f = flags_internal::g_head;
                 f != nullptr; f = f->Next()) {
                std::printf("  --%-20s (%s)  %s [default: %.*s]\n",
                            f->Name().data(),
                            f->TypeName().data(),
                            f->Help().data(),
                            static_cast<int>(f->CurrentValue().size()),
                            f->CurrentValue().data());
            }
            std::exit(0);
        }

        if (arg.size() < 2 || arg[0] != '-') {
            argv[write++] = argv[i];  // keep positional
            continue;
        }
        if (arg[1] != '-') {
            argv[write++] = argv[i];  // keep short flags (e.g. -v)
            continue;
        }
        // arg starts with "--"

        // ── --no<flag> (negate bool) ────────────────────────────────
        bool negate = false;
        std::string_view name = arg.substr(2);
        if (name.size() > 2 && name.substr(0, 2) == "no") {
            negate = true;
            name = name.substr(2);
        }

        // ── Parse --flag=value or --flag value ──────────────────────
        std::string_view value;
        size_t eq = name.find('=');
        if (eq != std::string_view::npos) {
            value = name.substr(eq + 1);
            name  = name.substr(0, eq);
        } else if (negate) {
            value = "false";
        } else if (i + 1 < argc) {
            // Try next argv as value if it doesn't start with '-'.
            std::string_view next(argv[i + 1]);
            if (!next.empty() && next[0] != '-') {
                value = next;
                ++i;
            } else {
                value = "true";  // bare --flag → true
            }
        } else {
            value = "true";
        }

        // ── Look up the flag in the registry ────────────────────────
        bool found = false;
        for (auto* f = flags_internal::g_head;
             f != nullptr; f = f->Next()) {
            if (f->Name() == name) {
                if (!f->Parse(value)) {
                    std::fprintf(stderr,
                        "ERROR: invalid value \"%.*s\" for flag --%.*s (type: %.*s)\n",
                        static_cast<int>(value.size()), value.data(),
                        static_cast<int>(name.size()), name.data(),
                        static_cast<int>(f->TypeName().size()),
                        f->TypeName().data());
                    std::exit(1);
                }
                found = true;
                break;
            }
        }
        if (!found) {
            std::fprintf(stderr,
                "ERROR: unknown flag --%.*s\n",
                static_cast<int>(name.size()), name.data());
            std::exit(1);
        }
    }

    argc = write;
    argv[argc] = nullptr;
}

} // namespace zeta

#endif // ZETA_FLAGS_PARSE_H
