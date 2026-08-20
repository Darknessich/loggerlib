#pragma once

#include <logger/Export.h>
#include <logger/ILogger.hpp>
#include <logger/LogLevel.hpp>

#include <memory>
#include <string>
#include <system_error>

namespace Logger {
    /// @brief Opens @p path for appending and returns a thread-safe logger writing to it.
    /// @param path   name of the log file; created when missing, never truncated
    /// @param level  initial threshold
    /// @param ec     failure reason; std::errc::invalid_argument for an out-of-range @p level
    /// @return the logger, or nullptr on failure
    /// @note Records go through formatRecord() and are flushed one by one; the error code of
    ///       log() is non-empty exactly on failure.
    LOGGER_EXPORT std::unique_ptr<ILogger>
    createFileLogger(const std::string& path, ELogLevel level, std::error_code& ec);
} // namespace Logger
