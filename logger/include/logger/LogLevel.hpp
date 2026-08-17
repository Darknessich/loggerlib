#pragma once

#include <logger/Export.h>

#include <optional>
#include <string_view>

namespace Logger {
    enum class ELogLevel {
        Debug = 0,
        Info,
        Warn,
        Error,
        Fatal,
        Count // Number of levels. Always last
    };

    LOGGER_EXPORT std::string_view level2string(ELogLevel level) noexcept;
    LOGGER_EXPORT std::optional<ELogLevel> string2level(std::string_view str) noexcept;
} // namespace Logger
