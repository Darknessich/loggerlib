#include "Options.hpp"

#include <common/Endpoint.hpp>

#include <optional>
#include <ostream>
#include <string_view>

namespace Collector {
    namespace {
        struct SScan {
            std::optional<std::string_view> listen;
            std::optional<std::string_view> countText;
            std::optional<std::string_view> timeoutText;
            std::optional<Logger::ESocketProtocol> protocol;
            std::size_t positional = 0;
        };

        std::optional<SUsageError> setListen(std::string_view text, SScan& scan) {
            if (scan.listen)
                return SUsageError{"listening address given twice: " + std::string{text}};

            scan.listen = text;
            return std::nullopt;
        }

        std::optional<SUsageError> setCount(std::string_view text, SScan& scan) {
            if (scan.countText) return SUsageError{"count given twice: " + std::string{text}};

            scan.countText = text;
            return std::nullopt;
        }

        std::optional<SUsageError> setTimeout(std::string_view text, SScan& scan) {
            if (scan.timeoutText) return SUsageError{"timeout given twice: " + std::string{text}};

            scan.timeoutText = text;
            return std::nullopt;
        }

        std::optional<SUsageError> setProtocol(std::string_view text, SScan& scan) {
            if (scan.protocol) return SUsageError{"protocol given twice: " + std::string{text}};

            const auto protocol = Logger::string2protocol(text);
            if (!protocol) return SUsageError{"unknown protocol: " + std::string{text}};

            scan.protocol = protocol;
            return std::nullopt;
        }

        using THandler = std::optional<SUsageError> (*)(std::string_view, SScan&);

        std::optional<THandler> handlerFor(std::string_view argument) {
            if (argument == "--listen") return &setListen;
            if (argument == "--proto") return &setProtocol;
            if (argument == "--count") return &setCount;
            if (argument == "--timeout") return &setTimeout;
            return std::nullopt;
        }

        std::optional<SUsageError> addPositional(std::string_view text, SScan& scan) {
            switch (scan.positional++) {
                case 0:
                    return setListen(text, scan);
                case 1:
                    return setCount(text, scan);
                case 2:
                    return setTimeout(text, scan);
                default:
                    return SUsageError{"unexpected argument: " + std::string{text}};
            }
        }

        void printProtocols(std::ostream& stream) {
            for (std::size_t i = 0; i < static_cast<std::size_t>(Logger::ESocketProtocol::Count);
                 ++i) {
                if (i > 0) stream << ", ";
                stream << Logger::protocol2string(static_cast<Logger::ESocketProtocol>(i));
            }
        }

        std::optional<SUsageError> readCounts(const SScan& scan, SRun& run) {
            if (scan.countText) {
                const auto count = Common::parseNumber<std::size_t>(*scan.countText);
                if (!count || *count == 0)
                    return SUsageError{
                        "count must be a positive number, got: " + std::string{*scan.countText}
                    };

                run.every = *count;
            }

            if (scan.timeoutText) {
                const auto seconds = Common::parseNumber<std::int64_t>(*scan.timeoutText);
                if (!seconds || *seconds <= 0)
                    return SUsageError{
                        "timeout must be a positive number of seconds, got: " +
                        std::string{*scan.timeoutText}
                    };

                run.timeout = std::chrono::seconds{*seconds};
            }

            return std::nullopt;
        }

        TOptions buildRun(const SScan& scan) {
            if (!scan.listen) return SUsageError{"a listening address is required"};

            const auto parts = Common::splitHostPort(*scan.listen);
            if (!parts)
                return SUsageError{
                    "expected <host:port>, got: " + std::string{*scan.listen} +
                    " (an IPv6 address needs brackets)"
                };

            const auto port = Common::parsePort(parts->port);
            if (!port) return SUsageError{"bad port: " + std::string{parts->port}};

            SRun run;
            run.host = std::string{parts->host};
            run.port = *port;
            if (scan.protocol) run.protocol = *scan.protocol;

            if (const auto error = readCounts(scan, run)) return *error;
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
                if (const auto error = addPositional(argument, scan)) return *error;
                continue;
            }

            if (argument == "--help" || argument == "-h") return SShowHelp{};

            const auto handler = handlerFor(argument);
            if (!handler) return SUsageError{"unknown option: " + std::string{argument}};

            if (index + 1 >= argc) return SUsageError{"missing value for " + std::string{argument}};

            if (const auto error = (*handler)(argv[++index], scan)) return *error;
        }

        return buildRun(scan);
    }

    void printUsage(const char* program, std::ostream& stream) {
        stream << "Usage: " << program
               << " [--] <host:port> [count] [timeout]\n"
                  "       "
               << program
               << " --listen <host:port> [--proto tcp|udp] [--count N] [--timeout T]\n"
                  "       "
               << program
               << " --help\n"
                  "\n"
                  "  <host:port>  address to listen on; \":5555\" takes every interface,\n"
                  "               an IPv6 address needs brackets: \"[::1]:5555\"\n"
                  "  --proto      transport to receive, one of ";
        printProtocols(stream);
        stream << " (default: " << Logger::protocol2string(SRun{}.protocol)
               << ")\n"
                  "  --count      report the statistics after every N-th message (default: "
               << SRun{}.every
               << ")\n"
                  "  --timeout    report them after T seconds as well, when anything arrived "
                  "(default: "
               << std::chrono::duration_cast<std::chrono::seconds>(SRun{}.timeout).count()
               << ")\n"
                  "\n"
                  "Received records are printed as they come. Stop with Ctrl-C for a final "
                  "report.\n";
    }
} // namespace Collector
