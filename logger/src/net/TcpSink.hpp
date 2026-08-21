#pragma once

#include "../ISink.hpp"

#include <common/net/Address.hpp>
#include <common/net/Socket.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Logger {
    struct STcpSettings {
        std::chrono::milliseconds connectTimeout{2000};
        std::chrono::milliseconds sendTimeout{2000};
        std::chrono::milliseconds initialCooldown{50};
        std::chrono::milliseconds maxCooldown{1000};
        unsigned resolveEvery{5};
    };

    class TcpSink final : public ISink {
    public:
        TcpSink(std::string host, std::uint16_t port, const STcpSettings& settings);

        bool connect(std::error_code& ec);
        bool writeLine(std::string_view line, std::error_code& ec) override;

    private:
        bool ensureConnected(std::error_code& ec);
        bool connectAny(std::error_code& ec);
        bool sendAll(std::string_view data, std::error_code& ec);
        void noteFailure(const std::error_code& ec);
        void noteSuccess();

        std::string m_host;
        std::uint16_t m_port;
        STcpSettings m_settings;

        std::vector<Common::SEndpoint> m_endpoints;
        Common::Socket m_socket;
        std::string m_frame;

        std::error_code m_lastError;
        std::chrono::steady_clock::time_point m_nextAttempt;
        std::chrono::milliseconds m_cooldown;
        unsigned m_failures = 0;
    };

    // Resolves and connects once
    std::unique_ptr<ISink> openTcpSink(
        const std::string& host,
        std::uint16_t port,
        const STcpSettings& settings,
        std::error_code& ec
    );
} // namespace Logger
