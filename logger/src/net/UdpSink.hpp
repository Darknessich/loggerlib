#pragma once

#include "../ISink.hpp"
#include "Socket.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace Logger {
    // One record per datagram
    class UdpSink final : public ISink {
    public:
        explicit UdpSink(Socket socket) noexcept;

        bool writeLine(std::string_view line, std::error_code& ec) override;

    private:
        Socket m_socket;
        std::string m_frame;
    };

    // Resolves and fixes the peer
    std::unique_ptr<ISink>
    openUdpSink(const std::string& host, std::uint16_t port, std::error_code& ec);
} // namespace Logger
