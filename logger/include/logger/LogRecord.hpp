#pragma once

#include <logger/Export.h>
#include <logger/LogLevel.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace Logger {
    /// @brief One entry of the log.
    struct SLogRecord {
        std::chrono::system_clock::time_point time; ///< When the message was accepted
        ELogLevel level{ELogLevel::Info};           ///< Severity
        std::string message;                        ///< Unescaped text
    };

    /// @brief Renders a record as the line `YYYY-MM-DD hh:mm:ss.mmmZ [LEVEL] message`.
    /// @param record record to render
    /// @return the line, without a trailing newline
    /// @note A time that cannot be formatted is written as `0000-00-00 00:00:00.000Z`.
    LOGGER_EXPORT std::string formatRecord(const SLogRecord& record);

    /// @brief Renders a record from its parts.
    /// @param time     moment to stamp the record with
    /// @param level    severity of the message
    /// @param message  text of the message
    /// @return the line, without a trailing newline
    LOGGER_EXPORT std::string formatRecord(
        std::chrono::system_clock::time_point time, ELogLevel level, std::string_view message
    );

    /// @brief Reads back a line produced by formatRecord().
    /// @param line one line of the log, without its terminator
    /// @return the record, or std::nullopt
    /// @note Dates before the epoch and beyond the range of system_clock are rejected.
    LOGGER_EXPORT std::optional<SLogRecord> parseRecord(std::string_view line);

    /// @brief Escapes the bytes that must not appear in a log line.
    /// @param message text to escape
    /// @return the escaped text
    /// @note Backslash, tab, carriage return and newline become `\\`, `\t`, `\r` and `\n`;
    ///       every other byte below 0x20, and 0x7F, becomes `\xNN`.
    LOGGER_EXPORT std::string escapeMessage(std::string_view message);

    /// @brief Undoes escapeMessage().
    /// @param message escaped text
    /// @return the original text; a broken escape sequence is kept as is
    LOGGER_EXPORT std::string unescapeMessage(std::string_view message);
} // namespace Logger
