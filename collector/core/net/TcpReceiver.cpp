#include "TcpReceiver.hpp"

#include <common/Errors.hpp>
#include <common/net/Address.hpp>

#include <array>
#include <cerrno>
#include <string_view>
#include <utility>

#include <poll.h>
#include <sys/socket.h>

namespace Collector {
    namespace {
        constexpr int kBacklog = 8;
        constexpr std::size_t kChunk = 4096;

        pollfd waiterFor(int fd) noexcept {
            pollfd waiter{};
            waiter.fd = fd;
            waiter.events = POLLIN;
            return waiter;
        }
    } // namespace

    TcpReceiver::TcpReceiver(SListener listener) noexcept : m_listener{std::move(listener)} {}

    bool TcpReceiver::receive(
        std::vector<std::string>& lines, std::chrono::milliseconds timeout, std::error_code& ec
    ) {
        std::vector<pollfd> waiters;
        waiters.reserve(m_clients.size() + 1);
        waiters.push_back(waiterFor(m_listener.socket.get()));
        for (const SClient& client : m_clients) {
            waiters.push_back(waiterFor(client.socket.get()));
        }

        errno = 0;
        const int ready = ::poll(waiters.data(), waiters.size(), static_cast<int>(timeout.count()));
        if (ready < 0) {
            if (errno == EINTR) return true;

            ec = Common::errnoError();
            return false;
        }
        if (ready == 0) return true;

        for (std::size_t i = m_clients.size(); i > 0; --i) {
            const auto events = waiters[i].revents;
            if ((events & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) == 0) continue;

            if (!read(m_clients[i - 1], lines)) {
                m_clients.erase(m_clients.begin() + static_cast<std::ptrdiff_t>(i - 1));
            }
        }

        if ((waiters.front().revents & POLLIN) != 0) acceptClients();
        return true;
    }

    void TcpReceiver::acceptClients() {
        while (m_clients.size() < kMaxClients) {
            Common::Socket client{::accept(m_listener.socket.get(), nullptr, nullptr)};
            if (!client.valid()) return;
            if (!client.setNonBlocking()) continue;

            m_clients.push_back(SClient{std::move(client), LineReader{}});
        }
    }

    bool TcpReceiver::read(SClient& client, std::vector<std::string>& lines) {
        std::array<char, kChunk> buffer{};

        errno = 0;
        const auto received = ::recv(client.socket.get(), buffer.data(), buffer.size(), 0);
        if (received > 0) {
            const std::string_view chunk{buffer.data(), static_cast<std::size_t>(received)};
            if (!client.reader.feed(chunk, lines)) ++m_dropped;
            return true;
        }

        if (received == 0) return false; // the sender closed the connection
        return errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK;
    }

    std::size_t TcpReceiver::dropped() const noexcept {
        return m_dropped;
    }

    std::uint16_t TcpReceiver::port() const noexcept {
        return m_listener.port;
    }

    const std::string& TcpReceiver::address() const noexcept {
        return m_listener.address;
    }

    std::unique_ptr<IReceiver>
    openTcpReceiver(const std::string& host, std::uint16_t port, std::error_code& ec) {
        auto listener = bindListener(host, port, SOCK_STREAM, kBacklog, ec);
        if (!listener) return nullptr;

        return std::make_unique<TcpReceiver>(std::move(*listener));
    }
} // namespace Collector
