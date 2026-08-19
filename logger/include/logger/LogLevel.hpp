#pragma once

#include <logger/Export.h>

#include <cstdint>
#include <optional>
#include <string_view>

namespace Logger {
    enum class ELogLevel : std::uint8_t {
        Debug = 0,
        Info,
        Warn,
        Error,
        Fatal,
        Count // Number of levels. Always last
    };

    LOGGER_EXPORT std::string_view level2string(ELogLevel level) noexcept;
    LOGGER_EXPORT std::optional<ELogLevel> string2level(std::string_view str) noexcept;
    LOGGER_EXPORT bool isValidLevel(ELogLevel level) noexcept;
} // namespace Logger
