#pragma once

#include "EventQueue.hpp"

#include <logger/ILogger.hpp>

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>

namespace App {
    // The only user of the logger. stop() returns once the queue is drained.
    class LogWorker {
    public:
        LogWorker(Logger::ILogger& logger, EventQueue& queue) noexcept;
        ~LogWorker();

        LogWorker(const LogWorker&) = delete;
        LogWorker& operator=(const LogWorker&) = delete;
        LogWorker(LogWorker&&) = delete;
        LogWorker& operator=(LogWorker&&) = delete;

        void start();
        void stop();

        [[nodiscard]] std::size_t processed() const noexcept;
        [[nodiscard]] std::size_t failed() const noexcept;
        [[nodiscard]] std::string lastError() const;

    private:
        void loop();
        void write(const SWrite& event);
        void recordFailure(std::string reason);

        Logger::ILogger& m_logger;
        EventQueue& m_queue;
        std::thread m_thread;
        std::atomic<std::size_t> m_processed{0};
        std::atomic<std::size_t> m_failed{0};
        mutable std::mutex m_errorMutex;
        std::string m_lastError;
    };
} // namespace App
