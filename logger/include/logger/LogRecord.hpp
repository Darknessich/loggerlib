#pragma once

#include <logger/Export.h>
#include <logger/LogLevel.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace Logger {
    struct SLogRecord {
        std::chrono::system_clock::time_point time;
        ELogLevel level;
        std::string message;
    };

    LOGGER_EXPORT std::string formatRecord(const SLogRecord& record);
    LOGGER_EXPORT std::optional<SLogRecord> parseRecord(std::string_view line);
    LOGGER_EXPORT std::string escapeMessage(std::string_view message);
    LOGGER_EXPORT std::string unescapeMessage(std::string_view message);
} // Logger