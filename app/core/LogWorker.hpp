#pragma once

#include "MessageQueue.hpp"

#include <logger/ILogger.hpp>

#include <atomic>
#include <cstddef>
#include <thread>

namespace App {
    class LogWorker {
    public:
        LogWorker(Logger::ILogger& logger, MessageQueue& queue) noexcept;
        ~LogWorker();

        LogWorker(const LogWorker&) = delete;
        LogWorker& operator=(const LogWorker&) = delete;
        LogWorker(LogWorker&&) = delete;
        LogWorker& operator=(LogWorker&&) = delete;

        void start();
        void stop();
        std::size_t processed() const noexcept;
        std::size_t failed() const noexcept;

    private:
        void loop();
        Logger::ILogger& m_logger;
        MessageQueue& m_queue;
        std::thread m_thread;
        std::atomic<std::size_t> m_processed{0};
        std::atomic<std::size_t> m_failed{0};
    };
} // namespace App
