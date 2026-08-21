#pragma once

#include <logger/Export.h>
#include <logger/ILogger.hpp>
#include <logger/LogLevel.hpp>
#include <logger/SocketProtocol.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <system_error>
#include <variant>
#include <vector>

namespace Logger {
    /// @brief A log file, created when missing and never truncated.
    struct SFileTarget {
        std::string path; ///< Name of the log file
    };

    /// @brief A receiver reachable over the network.
    struct SSocketTarget {
        std::string host;                               ///< Name or address
        std::uint16_t port{0};                          ///< Port, not zero
        ESocketProtocol protocol{ESocketProtocol::Tcp}; ///< Transport to speak
    };

    using TTarget = std::variant<SFileTarget, SSocketTarget>;

    /// @brief Creates a logger writing every record to each of @p targets.
    /// @param targets  destinations, at least one
    /// @param level    initial threshold
    /// @param ec       failure reason, std::errc::invalid_argument for an empty list or an
    ///                 out-of-range @p level
    /// @return the logger, or nullptr on failure
    /// @note A record is formatted once and offered to every target. The write fails when any
    ///       of them fails, and ec then names the first failure.
    LOGGER_EXPORT std::unique_ptr<ILogger>
    createLogger(const std::vector<TTarget>& targets, ELogLevel level, std::error_code& ec);

    /// @brief Creates a thread-safe logger writing every record to @p target.
    /// @param target  destination
    /// @param level   initial threshold
    /// @param ec      failure reason, a name that cannot be resolved carries a code of the
    ///                getaddrinfo category
    /// @return the logger, or nullptr on failure
    /// @note A file is created when missing, never truncated, and flushed after every record.
    ///       A socket carries the same lines a file would hold, each terminated by a newline:
    ///       over TCP a lost connection is retried on later records, over UDP delivery is not
    ///       acknowledged at all.
    LOGGER_EXPORT std::unique_ptr<ILogger>
    createLogger(TTarget target, ELogLevel level, std::error_code& ec);
} // namespace Logger
