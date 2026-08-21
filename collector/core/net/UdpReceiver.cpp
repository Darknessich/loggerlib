#include "UdpReceiver.hpp"

#include <common/Errors.hpp>
#include <common/net/Address.hpp>

#include <array>
#include <cerrno>
#include <cstddef>
#include <string_view>
#include <utility>

#include <poll.h>
#include <sys/socket.h>

namespace Collector {
    namespace {
        constexpr std::size_t kDatagram = 65536;

        void splitDatagram(std::string_view data, std::vector<std::string>& lines) {
            while (!data.empty()) {
                const auto end = data.find('\n');
                if (end == std::string_view::npos) {
                    lines.emplace_back(data);
                    return;
                }

                lines.emplace_back(data.substr(0, end));
                data.remove_prefix(end + 1);
            }
        }
    } // namespace

    UdpReceiver::UdpReceiver(SListener listener) noexcept : m_listener{std::move(listener)} {}

    bool UdpReceiver::receive(
        std::vector<std::string>& lines, std::chrono::milliseconds timeout, std::error_code& ec
    ) {
        pollfd waiter{};
        waiter.fd = m_listener.socket.get();
        waiter.events = POLLIN;

        errno = 0;
        const int ready = ::poll(&waiter, 1, static_cast<int>(timeout.count()));
        if (ready < 0) {
            if (errno == EINTR) return true;

            ec = Common::errnoError();
            return false;
        }
        if (ready == 0) return true;

        std::array<char, kDatagram> buffer{};
        errno = 0;
        const auto received = ::recv(m_listener.socket.get(), buffer.data(), buffer.size(), 0);
        if (received > 0) {
            splitDatagram({buffer.data(), static_cast<std::size_t>(received)}, lines);
            return true;
        }

        if (received < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK &&
            errno != ECONNREFUSED) {
            ec = Common::errnoError();
            return false;
        }

        return true;
    }

    std::size_t UdpReceiver::dropped() const noexcept {
        return 0;
    }

    std::uint16_t UdpReceiver::port() const noexcept {
        return m_listener.port;
    }

    const std::string& UdpReceiver::address() const noexcept {
        return m_listener.address;
    }

    std::unique_ptr<IReceiver>
    openUdpReceiver(const std::string& host, std::uint16_t port, std::error_code& ec) {
        auto listener = bindListener(host, port, SOCK_DGRAM, 0, ec);
        if (!listener) return nullptr;

        return std::make_unique<UdpReceiver>(std::move(*listener));
    }
} // namespace Collector
