#include "TcpSink.hpp"

#include <algorithm>
#include <cerrno>
#include <utility>

#include <poll.h>
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

        // A dead peer must not kill the host application
        void suppressSigpipe([[maybe_unused]] const Socket& socket) {
#ifdef SO_NOSIGPIPE
            const int on = 1;
            (void)::setsockopt(socket.get(), SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif
        }

        bool waitReady(int fd, std::chrono::milliseconds timeout) {
            pollfd waiter{};
            waiter.fd = fd;
            waiter.events = POLLOUT;

            int ready = 0;
            for (bool retry = true; retry;) {
                ready = ::poll(&waiter, 1, static_cast<int>(timeout.count()));
                retry = ready < 0 && errno == EINTR; // a signal, not an answer
            }

            return ready > 0;
        }
    } // namespace

    TcpSink::TcpSink(std::string host, std::uint16_t port, const STcpSettings& settings)
        : m_host{std::move(host)}, m_port{port}, m_settings{settings},
          m_cooldown{settings.initialCooldown} {}

    bool TcpSink::connect(std::error_code& ec) {
        m_endpoints = resolve(m_host, m_port, SOCK_STREAM, ec);
        if (ec) return false;

        if (connectAny(ec)) {
            noteSuccess();
            return true;
        }

        noteFailure(ec);
        return false;
    }

    bool TcpSink::connectAny(std::error_code& ec) {
        ec = std::make_error_code(std::errc::host_unreachable);

        for (const auto& endpoint : m_endpoints) {
            errno = 0;
            Socket socket{::socket(endpoint.family, endpoint.socktype, endpoint.protocol)};
            if (!socket.valid() || !socket.setNonBlocking()) {
                ec = lastError();
                continue;
            }
            suppressSigpipe(socket);

            const auto* address = reinterpret_cast<const sockaddr*>(&endpoint.address);
            errno = 0;
            if (::connect(socket.get(), address, endpoint.length) == 0) {
                m_socket = std::move(socket);
                ec.clear();
                return true;
            }

            if (errno != EINPROGRESS) {
                ec = lastError();
                continue;
            }

            if (!waitReady(socket.get(), m_settings.connectTimeout)) {
                ec = std::make_error_code(std::errc::timed_out);
                continue;
            }

            int pending = 0;
            auto length = static_cast<socklen_t>(sizeof(pending));
            if (::getsockopt(socket.get(), SOL_SOCKET, SO_ERROR, &pending, &length) != 0) {
                ec = lastError();
                continue;
            }
            if (pending != 0) {
                ec = std::error_code{pending, std::generic_category()};
                continue;
            }

            m_socket = std::move(socket);
            ec.clear();
            return true;
        }

        return false;
    }

    bool TcpSink::ensureConnected(std::error_code& ec) {
        if (m_socket.valid()) return true;

        if (std::chrono::steady_clock::now() < m_nextAttempt) {
            ec = m_lastError;
            return false;
        }

        if (m_settings.resolveEvery > 0 && m_failures % m_settings.resolveEvery == 0) {
            std::error_code resolveError;
            auto fresh = resolve(m_host, m_port, SOCK_STREAM, resolveError);
            if (!resolveError) m_endpoints = std::move(fresh);
        }

        if (connectAny(ec)) {
            noteSuccess();
            return true;
        }

        noteFailure(ec);
        return false;
    }

    bool TcpSink::sendAll(std::string_view data, std::error_code& ec) {
        std::size_t sent = 0;
        while (sent < data.size()) {
            errno = 0;
            const auto written =
                ::send(m_socket.get(), data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
            if (written > 0) {
                sent += static_cast<std::size_t>(written);
                continue;
            }

            if (errno == EINTR) continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (waitReady(m_socket.get(), m_settings.sendTimeout)) continue;
                ec = std::make_error_code(std::errc::timed_out);
                return false;
            }

            ec = lastError();
            return false;
        }

        return true;
    }

    bool TcpSink::writeLine(std::string_view line, std::error_code& ec) {
        if (!ensureConnected(ec)) return false;

        m_frame.assign(line);
        m_frame += '\n';
        if (sendAll(m_frame, ec)) return true;

        m_socket.reset();
        noteFailure(ec);
        return false;
    }

    void TcpSink::noteFailure(const std::error_code& ec) {
        m_lastError = ec;
        ++m_failures;
        m_nextAttempt = std::chrono::steady_clock::now() + m_cooldown;
        m_cooldown = std::min(m_cooldown * 2, m_settings.maxCooldown);
    }

    void TcpSink::noteSuccess() {
        m_lastError.clear();
        m_failures = 0;
        m_cooldown = m_settings.initialCooldown;
        m_nextAttempt = std::chrono::steady_clock::time_point{};
    }

    std::unique_ptr<ISink> openTcpSink(
        const std::string& host,
        std::uint16_t port,
        const STcpSettings& settings,
        std::error_code& ec
    ) {
        auto sink = std::make_unique<TcpSink>(host, port, settings);
        if (!sink->connect(ec)) return nullptr;
        return sink;
    }
} // namespace Logger
