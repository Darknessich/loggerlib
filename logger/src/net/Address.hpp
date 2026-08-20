#pragma once

#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

#include <netdb.h>
#include <sys/socket.h>

namespace Logger {
    struct SEndpoint {
        int family = 0;
        int socktype = 0;
        int protocol = 0;
        sockaddr_storage address{};
        socklen_t length = 0;
    };

    std::vector<SEndpoint>
    resolve(const std::string& host, std::uint16_t port, int socktype, std::error_code& ec);
} // namespace Logger
