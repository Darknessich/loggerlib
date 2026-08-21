#pragma once

#include <common/net/Socket.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <system_error>

namespace Collector {
    struct SListener {
        Common::Socket socket;
        std::string address;
        std::uint16_t port{0};
    };

    std::optional<SListener> bindListener(
        const std::string& host, std::uint16_t port, int socktype, int backlog, std::error_code& ec
    );
} // namespace Collector
