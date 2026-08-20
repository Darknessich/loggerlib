#include "Options.hpp"

#include <cstddef>
#include <optional>
#include <ostream>
#include <string_view>

namespace App {
    TOptions parseOptions(int argc, const char* const* argv) {
        std::optional<std::string_view> pathText;
        std::optional<std::string_view> levelText;

        bool optionsEnded = false;

        for (int index = 1; index < argc; ++index) {
            const std::string_view argument{argv[index]};

            if (!optionsEnded) {
                if (argument == "--") {
                    optionsEnded = true;
                    continue;
                }

                if (argument == "--help" || argument == "-h") return SShowHelp{};

                if (!argument.empty() && argument.front() == '-')
                    return SUsageError{"unknown option: " + std::string{argument}};
            }

            if (!pathText) {
                pathText = argument;
            } else if (!levelText) {
                levelText = argument;
            } else {
                return SUsageError{"unexpected argument: " + std::string{argument}};
            }
        }

        if (!pathText) return SUsageError{"log file name is required"};
        if (pathText->empty()) return SUsageError{"log file name must not be empty"};

        SRun run{std::string{*pathText}, Logger::ELogLevel::Info};
        if (levelText) {
            const auto level = Logger::string2level(*levelText);
            if (!level) return SUsageError{"unknown level: " + std::string{*levelText}};
            run.level = *level;
        }

        return run;
    }

    void printUsage(const char* program, std::ostream& stream) {
        stream << "Usage: " << program
               << " [--] <logfile> [level]\n"
                  "       "
               << program
               << " --help\n"
                  "\n"
                  "  <logfile>  file the log is appended to\n"
                  "  [level]    lowest level that reaches the log, one of ";
        printLevels(stream);
        stream << " (default: " << Logger::level2string(SRun{}.level)
               << ")\n"
                  "\n"
                  "Type /help inside the application for the list of commands.\n";
    }

    void printLevels(std::ostream& stream) {
        for (std::size_t i = 0; i < static_cast<std::size_t>(Logger::ELogLevel::Count); ++i) {
            if (i > 0) stream << ", ";
            stream << Logger::level2string(static_cast<Logger::ELogLevel>(i));
        }
    }
} // namespace App
