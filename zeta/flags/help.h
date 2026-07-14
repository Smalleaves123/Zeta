#ifndef ZETA_FLAGS_HELP_H
#define ZETA_FLAGS_HELP_H

/// @file   flags/help.h
/// @brief  Stable, generated help documentation for registered flags.

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "zeta/flags/internal/registry.h"

namespace zeta {

/// Returns deterministic help text sorted by flag name.
inline std::string FormatFlagsHelp(std::string_view title = "Flags") {
    std::vector<flags_internal::FlagEntry*> entries;
    for (auto* flag = flags_internal::FlagEntry::Head(); flag != nullptr;
         flag = flag->Next()) {
        entries.push_back(flag);
    }
    std::sort(entries.begin(), entries.end(), [](const auto* lhs, const auto* rhs) {
        return lhs->Name() < rhs->Name();
    });

    std::ostringstream out;
    out << title << ":\n";
    for (const auto* flag : entries) {
        out << "  --" << flag->Name() << "=<" << flag->TypeName() << ">\n"
            << "      " << flag->Help()
            << " [default: " << flag->DefaultValue() << ']';
        if (!flag->EnvName().empty()) {
            out << " [env: " << flag->EnvName() << ']';
        }
        if (flag->Required()) out << " [required]";
        out << '\n';
    }
    return out.str();
}

inline void PrintFlagsHelp(FILE* output = stdout,
                           std::string_view title = "Flags") {
    const std::string help = FormatFlagsHelp(title);
    std::fwrite(help.data(), 1, help.size(), output);
}

} // namespace zeta

#endif // ZETA_FLAGS_HELP_H
