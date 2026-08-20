#pragma once

#include <logger/SocketProtocol.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace utils {
    class LoopbackServer {
    public:
        explicit LoopbackServer(Logger::ESocketProtocol protocol);
        ~LoopbackServer();

        LoopbackServer(const LoopbackServer&) = delete;
        LoopbackServer& operator=(const LoopbackServer&) = delete;
        LoopbackServer(LoopbackServer&&) = delete;
        LoopbackServer& operator=(LoopbackServer&&) = delete;

        [[nodiscard]] std::uint16_t port() const noexcept { return m_port; }
        [[nodiscard]] bool started() const noexcept { return m_port != 0; }

        bool waitForLines(std::size_t count, std::chrono::milliseconds timeout);
        [[nodiscard]] std::vector<std::string> lines() const;

        void dropConnection();
        bool waitForDisconnect(std::chrono::milliseconds timeout);
        void stop();

    private:
        void serveStream();
        void serveDatagrams();
        void addLine(std::string line);

        int m_listener = -1;
        std::atomic<int> m_client{-1};
        std::uint16_t m_port = 0;

        std::atomic<bool> m_stopping{false};
        std::thread m_thread;

        mutable std::mutex m_mutex;
        std::condition_variable m_arrived;
        std::vector<std::string> m_lines;
    };
} // namespace utils
