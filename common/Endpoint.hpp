#pragma once

#include <common/Text.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace Common {
    struct SHostPort {
        std::string_view host;
        std::string_view port;
    };

    /// @brief Splits `host:port`, in brackets when the host is an IPv6 address.
    /// @param text argument to split, e.g. `127.0.0.1:5555`, `[::1]:5555` or `:5555`
    /// @return the halves or std::nullopt
    inline std::optional<SHostPort> splitHostPort(std::string_view text) {
        if (!text.empty() && text.front() == '[') {
            const auto close = text.find("]:");
            if (close == std::string_view::npos) return std::nullopt;
            return SHostPort{text.substr(1, close - 1), text.substr(close + 2)};
        }

        const auto colon = text.rfind(':');
        if (colon == std::string_view::npos) return std::nullopt;

        const std::string_view host = text.substr(0, colon);
        if (host.find(':') != std::string_view::npos) return std::nullopt;

        return SHostPort{host, text.substr(colon + 1)};
    }

    /// @brief Puts a host and a port back together, the inverse of splitHostPort().
    /// @param host host to name
    /// @param port port to name
    /// @return `host:port`, bracketed when the host is an IPv6 address
    inline std::string joinHostPort(std::string_view host, std::uint16_t port) {
        const bool bracketed = host.find(':') != std::string_view::npos;
        std::string text;

        if (bracketed) text += '[';
        text += host;
        if (bracketed) text += ']';

        text += ':';
        text += std::to_string(port);
        return text;
    }

    /// @brief Reads a port number.
    /// @param text digits to read
    /// @return the port, or std::nullopt when it is not a number or is zero
    inline std::optional<std::uint16_t> parsePort(std::string_view text) {
        const auto port = parseNumber<std::uint16_t>(text);
        if (!port || *port == 0) return std::nullopt;
        return port;
    }
} // namespace Common
