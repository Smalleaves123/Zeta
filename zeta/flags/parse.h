#ifndef ZETA_FLAGS_PARSE_H
#define ZETA_FLAGS_PARSE_H

/// @file   flags/parse.h
/// @brief  Command-line parser for registered flags.

#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <vector>

#include "zeta/flags/flag.h"
#include "zeta/flags/internal/registry.h"

namespace zeta {

/// Parse command-line flags.  Non-flag arguments are returned in `rest`.
///
/// Supported syntax:
///   --flag=value   — set any flag to value
///   --flag         — bool flag set to true; non-bool call Parse("true")
///   --noflag       — bool flag set to false
///   --help / -h    — print all registered flags and exit
///
/// Returns positional (non-flag) arguments.  Does NOT modify argc/argv.
inline std::vector<std::string_view> ParseCommandLine(int argc, char** argv) {
    std::vector<std::string_view> rest;
    rest.reserve(static_cast<size_t>(argc));

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);

        // ── --help ──────────────────────────────────────────────────
        if (arg == "--help" || arg == "-h") {
            std::printf("Flags:\n");
            for (auto* f = flags_internal::FlagEntry::Head();
                 f != nullptr; f = f->Next()) {
                const std::string_view current = f->CurrentValue();
                std::printf("  --%-20s (%s)  %s [default: %.*s]\n",
                            f->Name().data(), f->TypeName().data(),
                            f->Help().data(),
                            static_cast<int>(current.size()),
                            current.data());
            }
            std::exit(0);
        }

        if (arg.size() < 3 || arg[0] != '-' || arg[1] != '-') {
            rest.push_back(arg);  // not a flag
            continue;
        }
        // arg starts with "--"

        // ── Parse --flag=value or bare --flag ───────────────────────
        std::string_view name = arg.substr(2);
        std::string_view value;
        size_t eq = name.find('=');
        if (eq != std::string_view::npos) {
            value = name.substr(eq + 1);
            name  = name.substr(0, eq);
        } else {
            value = "true";
        }

        // ── Look up in registry ────────────────────────────────────
        bool found = false;
        for (auto* f = flags_internal::FlagEntry::Head();
             f != nullptr; f = f->Next()) {
            if (f->Name() == name) {
                if (!f->Parse(value)) {
                    std::fprintf(stderr,
                        "ERROR: invalid value \"%.*s\" for --%.*s (type: %.*s)\n",
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
        if (!found && eq == std::string_view::npos && name.size() > 2 &&
            name.substr(0, 2) == "no") {
            std::string_view stripped = name.substr(2);
            for (auto* f = flags_internal::FlagEntry::Head();
                 f != nullptr; f = f->Next()) {
                if (f->Name() == stripped) {
                    if (!f->Parse("false")) {
                        std::fprintf(stderr,
                            "ERROR: invalid value \"false\" for --%.*s (type: %.*s)\n",
                            static_cast<int>(stripped.size()), stripped.data(),
                            static_cast<int>(f->TypeName().size()),
                            f->TypeName().data());
                        std::exit(1);
                    }
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            std::fprintf(stderr, "ERROR: unknown flag --%.*s\n",
                         static_cast<int>(name.size()), name.data());
            std::exit(1);
        }
    }
    return rest;
}

} // namespace zeta

#endif // ZETA_FLAGS_PARSE_H
