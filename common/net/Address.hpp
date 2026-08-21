#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <netdb.h>
#include <sys/socket.h>

namespace Common {
    /// @brief One address a name resolved to, ready for socket(), connect() and bind().
    struct SEndpoint {
        int family = 0;             ///< AF_INET or AF_INET6
        int socktype = 0;           ///< SOCK_STREAM or SOCK_DGRAM
        int protocol = 0;           ///< Protocol number for socket()
        sockaddr_storage address{}; ///< The address
        socklen_t length = 0;       ///< Length of address
    };

    /// @brief Resolves @p host and @p port into the addresses they name.
    /// @param host       name or address, empty for the wildcard
    /// @param port       port number
    /// @param socktype   SOCK_STREAM or SOCK_DGRAM
    /// @param ec         failure reason
    /// @param extraFlags added to the hints, AI_PASSIVE to bind the result
    /// @return the addresses in the resolver's order, empty on failure
    std::vector<SEndpoint> resolve(
        const std::string& host,
        std::uint16_t port,
        int socktype,
        std::error_code& ec,
        int extraFlags = 0
    );

    /// @brief Returns the address of @p endpoint as the socket calls take it.
    /// @param endpoint endpoint to point at
    /// @return pointer into @p endpoint
    const sockaddr* asSockaddr(const SEndpoint& endpoint) noexcept;

    /// @brief Asks which port @p fd is bound to.
    /// @param fd bound descriptor
    /// @return the port or std::nullopt
    std::optional<std::uint16_t> localPort(int fd) noexcept;

    /// @brief Asks which address @p fd is bound to.
    /// @param fd bound descriptor
    /// @return the address as text, `0.0.0.0` or `::` for every interface
    std::optional<std::string> localAddress(int fd);
} // namespace Common
