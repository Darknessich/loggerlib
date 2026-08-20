#include "UdpSink.hpp"
#include "Address.hpp"

#include <cerrno>
#include <utility>

#include <sys/socket.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace Logger {
    namespace {
        std::error_code lastError() {
            return errno != 0 ? std::error_code{errno, std::generic_category()}
                              : std::make_error_code(std::errc::io_error);
        }
    } // namespace

    UdpSink::UdpSink(Socket socket) noexcept : m_socket{std::move(socket)} {}

    bool UdpSink::writeLine(std::string_view line, std::error_code& ec) {
        m_frame.assign(line);
        m_frame += '\n';

        ssize_t sent = 0;
        for (bool retry = true; retry;) {
            sent = ::send(m_socket.get(), m_frame.data(), m_frame.size(), MSG_NOSIGNAL);
            retry = sent < 0 && errno == EINTR;
        }

        if (sent == static_cast<ssize_t>(m_frame.size())) return true;

        ec = sent < 0 ? lastError() : std::make_error_code(std::errc::message_size);
        return false;
    }

    std::unique_ptr<ISink>
    openUdpSink(const std::string& host, std::uint16_t port, std::error_code& ec) {
        const auto endpoints = resolve(host, port, SOCK_DGRAM, ec);
        if (ec) return nullptr;

        ec = std::make_error_code(std::errc::host_unreachable);
        for (const auto& endpoint : endpoints) {
            errno = 0;
            Socket socket{::socket(endpoint.family, endpoint.socktype, endpoint.protocol)};
            if (!socket.valid()) {
                ec = lastError();
                continue;
            }

            const auto* address = reinterpret_cast<const sockaddr*>(&endpoint.address);
            errno = 0;
            if (::connect(socket.get(), address, endpoint.length) != 0) {
                ec = lastError();
                continue;
            }

            ec.clear();
            return std::make_unique<UdpSink>(std::move(socket));
        }

        return nullptr;
    }
} // namespace Logger
