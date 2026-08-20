#pragma once

#include <logger/Export.h>

#include <cstdint>
#include <optional>
#include <string_view>

namespace Logger {
    /// @brief Severity of a message, ordered from the least to the most important.
    enum class ELogLevel : std::uint8_t {
        Debug = 0,
        Info,
        Warn,
        Error,
        Fatal,
        Count ///< Number of levels. Always last
    };

    /// @brief Returns the name of @p level.
    /// @param level level to name
    /// @return upper-case name, or "UNKNOWN" when @p level is out of range
    LOGGER_EXPORT std::string_view level2string(ELogLevel level) noexcept;

    /// @brief Parses a level name, the inverse of level2string().
    /// @param str name to parse, case-insensitive
    /// @return the level, or std::nullopt
    LOGGER_EXPORT std::optional<ELogLevel> string2level(std::string_view str) noexcept;

    /// @brief Returns true when @p level names a level.
    /// @param level value to check
    /// @return false for Count and for any value beyond it
    LOGGER_EXPORT bool isValidLevel(ELogLevel level) noexcept;
} // namespace Logger
