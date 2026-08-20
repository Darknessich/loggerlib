#pragma once

#include <logger/Export.h>

#include <cstdint>
#include <optional>
#include <string_view>

namespace Logger {
    /// @brief Transport a socket logger speaks.
    enum class ESocketProtocol : std::uint8_t {
        Tcp = 0,
        Udp,
        Count ///< Number of protocols. Always last
    };

    /// @brief Returns the name of @p protocol.
    /// @param protocol protocol to name
    /// @return lower-case name, or "unknown" when @p protocol is out of range
    LOGGER_EXPORT std::string_view protocol2string(ESocketProtocol protocol) noexcept;

    /// @brief Parses a protocol name, the inverse of protocol2string().
    /// @param str name to parse, case-insensitive
    /// @return the protocol, or std::nullopt
    LOGGER_EXPORT std::optional<ESocketProtocol> string2protocol(std::string_view str) noexcept;

    /// @brief Returns true when @p protocol names a protocol.
    /// @param protocol value to check
    /// @return false for Count and for any value beyond it
    LOGGER_EXPORT bool isValidProtocol(ESocketProtocol protocol) noexcept;
} // namespace Logger
