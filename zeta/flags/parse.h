#ifndef ZETA_FLAGS_PARSE_H
#define ZETA_FLAGS_PARSE_H

/// @file   flags/parse.h
/// @brief  Command-line and environment parser for registered flags.

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "zeta/flags/flag.h"
#include "zeta/flags/help.h"
#include "zeta/flags/internal/registry.h"

namespace zeta {

struct FlagValidationError {
    std::string name;
    std::string message;
};

struct FlagParseResult {
    std::vector<std::string_view> positional;
    std::vector<std::string> errors;
    bool help_requested = false;

    [[nodiscard]] bool ok() const noexcept { return errors.empty(); }
};

inline flags_internal::FlagEntry* FindFlag(std::string_view name) noexcept {
    for (auto* flag = flags_internal::FlagEntry::Head(); flag != nullptr;
         flag = flag->Next()) {
        if (flag->Name() == name) return flag;
    }
    return nullptr;
}

/// Validates all registered flags, including required and custom validators.
inline std::vector<FlagValidationError> ValidateFlags() {
    std::vector<FlagValidationError> errors;
    for (auto* flag = flags_internal::FlagEntry::Head(); flag != nullptr;
         flag = flag->Next()) {
        std::string message;
        if (!flag->Validate(&message)) {
            errors.push_back({std::string(flag->Name()), std::move(message)});
        }
    }
    return errors;
}

/// Applies environment values before command-line values.
/// Command-line arguments therefore take precedence over the environment.
inline std::vector<std::string> ApplyEnvironmentVariables() {
    std::vector<std::string> errors;
    for (auto* flag = flags_internal::FlagEntry::Head(); flag != nullptr;
         flag = flag->Next()) {
        if (flag->EnvName().empty()) continue;
        const char* value = std::getenv(std::string(flag->EnvName()).c_str());
        if (value == nullptr) continue;
        if (!flag->Parse(value)) {
            errors.push_back("invalid environment value for " +
                             std::string(flag->EnvName()) + " (flag --" +
                             std::string(flag->Name()) + ")");
        }
    }
    return errors;
}

inline void AppendFlagError(FlagParseResult* result,
                            std::string_view message) {
    result->errors.emplace_back(message);
}

/// Parses flags without exiting. Useful for services and tests that need to
/// report all configuration errors together.
inline FlagParseResult ParseCommandLineChecked(int argc, char** argv) {
    FlagParseResult result;
    for (auto& error : ApplyEnvironmentVariables()) {
        result.errors.push_back(std::move(error));
    }

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--") {
            for (++i; i < argc; ++i) result.positional.emplace_back(argv[i]);
            break;
        }
        if (arg == "--help" || arg == "-h") {
            result.help_requested = true;
            continue;
        }
        if (arg.size() < 3 || arg[0] != '-' || arg[1] != '-') {
            result.positional.push_back(arg);
            continue;
        }

        std::string_view name = arg.substr(2);
        const std::size_t equals = name.find('=');
        const bool has_equals = equals != std::string_view::npos;
        std::string_view value;
        if (has_equals) {
            value = name.substr(equals + 1);
            name = name.substr(0, equals);
        }

        flags_internal::FlagEntry* flag = FindFlag(name);
        if (flag == nullptr && !has_equals && name.size() > 2 &&
            name.substr(0, 2) == "no") {
            const std::string_view stripped = name.substr(2);
            flag = FindFlag(stripped);
            if (flag != nullptr && flag->TypeName() == "bool") {
                value = "false";
                name = stripped;
            } else {
                flag = nullptr;
            }
        }

        if (flag == nullptr) {
            AppendFlagError(&result, "unknown flag --" + std::string(name));
            continue;
        }

        if (!has_equals && value.empty()) {
            if (flag->TypeName() == "bool") {
                value = "true";
            } else if (i + 1 < argc &&
                       std::string_view(argv[i + 1]).substr(0, 2) != "--") {
                value = argv[++i];
            } else {
                AppendFlagError(&result, "missing value for --" +
                                             std::string(name));
                continue;
            }
        }

        if (!flag->Parse(value)) {
            AppendFlagError(&result, "invalid value for --" +
                                         std::string(name) + ": " +
                                         std::string(value));
        }
    }

    if (!result.help_requested) {
        for (const auto& error : ValidateFlags()) {
            result.errors.push_back("--" + error.name + ": " + error.message);
        }
    }
    return result;
}

/// Parse command-line flags and exit with a diagnostic on invalid input.
/// Non-flag arguments are returned in `rest`.
inline std::vector<std::string_view> ParseCommandLine(int argc, char** argv) {
    const FlagParseResult result = ParseCommandLineChecked(argc, argv);
    if (result.help_requested) {
        PrintFlagsHelp();
        std::exit(0);
    }
    if (!result.errors.empty()) {
        for (const auto& error : result.errors) {
            std::fprintf(stderr, "ERROR: %s\n", error.c_str());
        }
        std::exit(1);
    }
    return result.positional;
}

} // namespace zeta

#endif // ZETA_FLAGS_PARSE_H
