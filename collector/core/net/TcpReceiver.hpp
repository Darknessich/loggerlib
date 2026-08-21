#pragma once

#include "../IReceiver.hpp"
#include "../LineReader.hpp"
#include "Listener.hpp"

#include <common/net/Socket.hpp>

#include <memory>
#include <string>
#include <vector>

namespace Collector {
    class TcpReceiver final : public IReceiver {
    public:
        explicit TcpReceiver(SListener listener) noexcept;

        bool receive(
            std::vector<std::string>& lines, std::chrono::milliseconds timeout, std::error_code& ec
        ) override;

        [[nodiscard]] std::size_t dropped() const noexcept override;
        [[nodiscard]] std::uint16_t port() const noexcept override;
        [[nodiscard]] const std::string& address() const noexcept override;

    private:
        static constexpr std::size_t kMaxClients = 16;

        struct SClient {
            Common::Socket socket;
            LineReader reader;
        };

        void acceptClients();
        bool read(SClient& client, std::vector<std::string>& lines);

        SListener m_listener;
        std::vector<SClient> m_clients;
        std::size_t m_dropped{0};
    };

    std::unique_ptr<IReceiver>
    openTcpReceiver(const std::string& host, std::uint16_t port, std::error_code& ec);
} // namespace Collector
