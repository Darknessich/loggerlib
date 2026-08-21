#pragma once

#include <logger/LogLevel.hpp>
#include <logger/LoggerFactory.hpp>

#include <iosfwd>
#include <string>
#include <variant>
#include <vector>

namespace App {
    struct SRun {
        std::vector<Logger::TTarget> targets;
        Logger::ELogLevel level{Logger::ELogLevel::Info};
    };

    struct SUsageError {
        std::string text;
    };

    struct SShowHelp {};

    using TOptions = std::variant<SRun, SShowHelp, SUsageError>;

    // A dash starts an option, "--" ends them
    TOptions parseOptions(int argc, const char* const* argv);
    void printUsage(const char* program, std::ostream& stream);
    void printLevels(std::ostream& stream);
} // namespace App
