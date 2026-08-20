#include <utils/LoopbackServer.hpp>

#include <array>
#include <utility>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace utils {
    namespace {
        constexpr int kPollTimeoutMs = 50;

        sockaddr_in loopbackAddress() {
            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            address.sin_port = 0;
            return address;
        }

        bool waitReadable(int fd) {
            pollfd waiter{};
            waiter.fd = fd;
            waiter.events = POLLIN;
            return ::poll(&waiter, 1, kPollTimeoutMs) > 0;
        }
    } // namespace

    LoopbackServer::LoopbackServer(Logger::ESocketProtocol protocol) {
        const bool stream = protocol == Logger::ESocketProtocol::Tcp;
        m_listener = ::socket(AF_INET, stream ? SOCK_STREAM : SOCK_DGRAM, 0);
        if (m_listener < 0) return;

        const int on = 1;
        (void)::setsockopt(m_listener, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

        sockaddr_in address = loopbackAddress();
        if (::bind(m_listener, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0)
            return;
        if (stream && ::listen(m_listener, 4) != 0) return;

        sockaddr_in bound{};
        auto length = static_cast<socklen_t>(sizeof(bound));
        if (::getsockname(m_listener, reinterpret_cast<sockaddr*>(&bound), &length) != 0) return;
        m_port = ntohs(bound.sin_port);

        m_thread = std::thread{
            stream ? &LoopbackServer::serveStream : &LoopbackServer::serveDatagrams, this
        };
    }

    LoopbackServer::~LoopbackServer() {
        stop();
    }

    void LoopbackServer::stop() {
        m_stopping.store(true);
        if (m_thread.joinable()) m_thread.join();

        const int client = m_client.exchange(-1);
        if (client >= 0) ::close(client);
        if (m_listener >= 0) ::close(m_listener);
        m_listener = -1;
    }

    void LoopbackServer::dropConnection() {
        const int client = m_client.load();
        if (client >= 0) ::shutdown(client, SHUT_RDWR);
    }

    bool LoopbackServer::waitForDisconnect(std::chrono::milliseconds timeout) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (m_client.load() < 0) return true;
            std::this_thread::yield();
        }
        return m_client.load() < 0;
    }

    void LoopbackServer::addLine(std::string line) {
        {
            const std::lock_guard lock(m_mutex);
            m_lines.push_back(std::move(line));
        }
        m_arrived.notify_all();
    }

    void LoopbackServer::serveStream() {
        std::string pending;

        while (!m_stopping.load()) {
            if (!waitReadable(m_listener)) continue;

            const int client = ::accept(m_listener, nullptr, nullptr);
            if (client < 0) continue;
            m_client.store(client);

            std::array<char, 4096> buffer{};
            while (!m_stopping.load()) {
                if (!waitReadable(client)) continue;

                const auto received = ::recv(client, buffer.data(), buffer.size(), 0);
                if (received <= 0) break;

                pending.append(buffer.data(), static_cast<std::size_t>(received));
                for (auto end = pending.find('\n'); end != std::string::npos;
                     end = pending.find('\n')) {
                    addLine(pending.substr(0, end));
                    pending.erase(0, end + 1);
                }
            }

            m_client.store(-1);
            ::close(client);
            pending.clear();
        }
    }

    void LoopbackServer::serveDatagrams() {
        std::array<char, 65536> buffer{};

        while (!m_stopping.load()) {
            if (!waitReadable(m_listener)) continue;

            const auto received = ::recv(m_listener, buffer.data(), buffer.size(), 0);
            if (received <= 0) continue;

            std::string line{buffer.data(), static_cast<std::size_t>(received)};
            if (!line.empty() && line.back() == '\n') line.pop_back();
            addLine(std::move(line));
        }
    }

    bool LoopbackServer::waitForLines(std::size_t count, std::chrono::milliseconds timeout) {
        std::unique_lock lock(m_mutex);
        return m_arrived.wait_for(lock, timeout, [this, count] { return m_lines.size() >= count; });
    }

    std::vector<std::string> LoopbackServer::lines() const {
        const std::lock_guard lock(m_mutex);
        return m_lines;
    }
} // namespace utils
