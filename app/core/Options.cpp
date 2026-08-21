#include "Options.hpp"

#include <common/Endpoint.hpp>

#include <cstddef>
#include <optional>
#include <ostream>
#include <string_view>
#include <utility>

namespace App {
    namespace {
        struct SScan {
            std::vector<Logger::TTarget> targets;
            std::optional<std::size_t> lastSocket; // --proto applies to this target
            std::optional<std::string_view> levelText;
            bool protocolGiven = false;
        };

        std::optional<SUsageError> addFile(std::string_view path, SScan& scan) {
            if (path.empty()) return SUsageError{"log file name must not be empty"};

            scan.targets.emplace_back(Logger::SFileTarget{std::string{path}});
            return std::nullopt;
        }

        std::optional<SUsageError> addSocket(std::string_view text, SScan& scan) {
            const auto parts = Common::splitHostPort(text);
            if (!parts)
                return SUsageError{
                    "expected <host:port>, got: " + std::string{text} +
                    " (an IPv6 address needs brackets)"
                };

            if (parts->host.empty()) return SUsageError{"host must not be empty"};

            const auto port = Common::parsePort(parts->port);
            if (!port) return SUsageError{"bad port: " + std::string{parts->port}};

            scan.lastSocket = scan.targets.size();
            scan.protocolGiven = false;
            scan.targets.emplace_back(Logger::SSocketTarget{std::string{parts->host}, *port, {}});
            return std::nullopt;
        }

        std::optional<SUsageError> setProtocol(std::string_view text, SScan& scan) {
            if (!scan.lastSocket) return SUsageError{"--proto requires a preceding --socket"};
            if (scan.protocolGiven) return SUsageError{"--proto given twice for one --socket"};

            const auto protocol = Logger::string2protocol(text);
            if (!protocol) return SUsageError{"unknown protocol: " + std::string{text}};

            std::get<Logger::SSocketTarget>(scan.targets[*scan.lastSocket]).protocol = *protocol;
            scan.protocolGiven = true;
            return std::nullopt;
        }

        std::optional<SUsageError> setLevel(std::string_view text, SScan& scan) {
            if (scan.levelText) return SUsageError{"level given twice: " + std::string{text}};

            scan.levelText = text;
            return std::nullopt;
        }

        using THandler = std::optional<SUsageError> (*)(std::string_view, SScan&);

        std::optional<THandler> handlerFor(std::string_view argument) {
            if (argument == "--file") return &addFile;
            if (argument == "--socket") return &addSocket;
            if (argument == "--proto") return &setProtocol;
            if (argument == "--level") return &setLevel;
            return std::nullopt;
        }

        void printProtocols(std::ostream& stream) {
            for (std::size_t i = 0; i < static_cast<std::size_t>(Logger::ESocketProtocol::Count);
                 ++i) {
                if (i > 0) stream << ", ";
                stream << Logger::protocol2string(static_cast<Logger::ESocketProtocol>(i));
            }
        }

        TOptions buildRun(SScan scan) {
            if (scan.targets.empty()) return SUsageError{"a log file or --socket is required"};

            SRun run;
            run.targets = std::move(scan.targets);
            if (!scan.levelText) return run;

            const auto level = Logger::string2level(*scan.levelText);
            if (!level) return SUsageError{"unknown level: " + std::string{*scan.levelText}};

            run.level = *level;
            return run;
        }
    } // namespace

    TOptions parseOptions(int argc, const char* const* argv) {
        SScan scan;
        bool optionsEnded = false;

        for (int index = 1; index < argc; ++index) {
            const std::string_view argument{argv[index]};

            if (!optionsEnded && argument == "--") {
                optionsEnded = true;
                continue;
            }

            if (optionsEnded || argument.empty() || argument.front() != '-') {
                const auto error =
                    scan.targets.empty() ? addFile(argument, scan) : setLevel(argument, scan);
                if (error) return *error;
                continue;
            }

            if (argument == "--help" || argument == "-h") return SShowHelp{};

            const auto handler = handlerFor(argument);
            if (!handler) return SUsageError{"unknown option: " + std::string{argument}};

            if (index + 1 >= argc) return SUsageError{"missing value for " + std::string{argument}};

            if (const auto error = (*handler)(argv[++index], scan)) return *error;
        }

        return buildRun(std::move(scan));
    }

    void printUsage(const char* program, std::ostream& stream) {
        stream << "Usage: " << program
               << " [--] <logfile> [level]\n"
                  "       "
               << program
               << " [--file <path>] [--socket <host:port> [--proto tcp|udp]] [--level <name>]\n"
                  "       "
               << program
               << " --help\n"
                  "\n"
                  "  <logfile>  file the log is appended to\n"
                  "  --file     another log file, may be given several times\n"
                  "  --socket   receiver the log is sent to, may be given several times\n"
                  "  --proto    transport of the preceding --socket, one of ";
        printProtocols(stream);
        stream << " (default: " << Logger::protocol2string(Logger::SSocketTarget{}.protocol)
               << ")\n"
                  "  --level    lowest level that reaches the log, one of ";
        printLevels(stream);
        stream << " (default: " << Logger::level2string(SRun{}.level)
               << ")\n"
                  "  [level]    same as --level\n"
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
