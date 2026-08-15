#pragma once

#include <logger/Export.h>

#include <optional>
#include <string_view>

namespace Logger {
    enum class ELogLevel {
        DEBUG = 0,
        INFO,
        WARN,
        ERROR,
        FATAL,
        COUNT // Number of levels. Always last
    };

    LOGGER_EXPORT std::string_view level2string(ELogLevel level) noexcept;
    LOGGER_EXPORT std::optional<ELogLevel> string2level(std::string_view str) noexcept;
} // Logger