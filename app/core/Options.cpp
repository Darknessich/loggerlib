#include "Options.hpp"

#include <cstddef>
#include <optional>
#include <ostream>
#include <string_view>

namespace App {
    SOptions parseOptions(int argc, const char* const* argv) {
        SOptions parsed;
        std::optional<std::string_view> pathText;
        std::optional<std::string_view> levelText;

        for (int index = 1; index < argc; ++index) {
            const std::string_view argument{argv[index]};

            if (argument == "--help" || argument == "-h") {
                parsed.kind = EOptionsKind::Help;
                return parsed;
            }

            if (!argument.empty() && argument.front() == '-') {
                parsed.error = "unknown option: " + std::string{argument};
                return parsed;
            }

            if (!pathText) {
                pathText = argument;
            } else if (!levelText) {
                levelText = argument;
            } else {
                parsed.error = "unexpected argument: " + std::string{argument};
                return parsed;
            }
        }

        if (!pathText) {
            parsed.error = "log file name is required";
            return parsed;
        }
        if (pathText->empty()) {
            parsed.error = "log file name must not be empty";
            return parsed;
        }

        if (levelText) {
            const auto level = Logger::string2level(*levelText);
            if (!level) {
                parsed.error = "unknown level: " + std::string{*levelText};
                return parsed;
            }
            parsed.level = *level;
        }

        parsed.path = std::string{*pathText};
        parsed.kind = EOptionsKind::Run;
        return parsed;
    }

    void printUsage(const char* program, std::ostream& stream) {
        stream << "Usage: " << program << " <logfile> [level]\n"
                  "       " << program << " --help\n"
                  "\n"
                  "  <logfile>  file the log is appended to\n"
                  "  [level]    lowest level that reaches the log, one of ";
        printLevels(stream);
        stream << " (default: " << Logger::level2string(SOptions{}.level) << ")\n"
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
