#pragma once

#include <logger/SocketProtocol.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <variant>

namespace Collector {
    struct SRun {
        std::string host;
        std::uint16_t port{0};
        Logger::ESocketProtocol protocol{Logger::ESocketProtocol::Tcp};
        std::size_t every{100};
        std::chrono::milliseconds timeout{std::chrono::seconds{10}};
    };

    struct SUsageError {
        std::string text;
    };

    struct SShowHelp {};

    using TOptions = std::variant<SRun, SShowHelp, SUsageError>;

    // A dash starts an option, "--" ends them.
    TOptions parseOptions(int argc, const char* const* argv);
    void printUsage(const char* program, std::ostream& stream);
} // namespace Collector
