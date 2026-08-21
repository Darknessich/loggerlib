#include "UdpSink.hpp"

#include <common/Errors.hpp>
#include <common/net/Address.hpp>

#include <cerrno>
#include <utility>

#include <sys/socket.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace Logger {
    UdpSink::UdpSink(Common::Socket socket) noexcept : m_socket{std::move(socket)} {}

    bool UdpSink::writeLine(std::string_view line, std::error_code& ec) {
        m_frame.assign(line);
        m_frame += '\n';

        ssize_t sent = 0;
        for (bool retry = true; retry;) {
            sent = ::send(m_socket.get(), m_frame.data(), m_frame.size(), MSG_NOSIGNAL);
            retry = sent < 0 && errno == EINTR;
        }

        if (sent == static_cast<ssize_t>(m_frame.size())) return true;

        ec = sent < 0 ? Common::errnoError() : std::make_error_code(std::errc::message_size);
        return false;
    }

    std::unique_ptr<ISink>
    openUdpSink(const std::string& host, std::uint16_t port, std::error_code& ec) {
        const auto endpoints = Common::resolve(host, port, SOCK_DGRAM, ec);
        if (ec) return nullptr;

        ec = std::make_error_code(std::errc::host_unreachable);
        for (const auto& endpoint : endpoints) {
            errno = 0;
            Common::Socket socket{::socket(endpoint.family, endpoint.socktype, endpoint.protocol)};
            if (!socket.valid()) {
                ec = Common::errnoError();
                continue;
            }

            const auto* address = Common::asSockaddr(endpoint);
            errno = 0;
            if (::connect(socket.get(), address, endpoint.length) != 0) {
                ec = Common::errnoError();
                continue;
            }

            ec.clear();
            return std::make_unique<UdpSink>(std::move(socket));
        }

        return nullptr;
    }
} // namespace Logger
