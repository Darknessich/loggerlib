#include "LogWorker.hpp"

namespace App {
    LogWorker::LogWorker(Logger::ILogger& logger, MessageQueue& queue) noexcept
        : m_logger{logger}, m_queue{queue} {}

    LogWorker::~LogWorker() {
        stop();
    }

    void LogWorker::start() {
        if (m_thread.joinable()) return;
        m_thread = std::thread{&LogWorker::loop, this};
    }

    void LogWorker::stop() {
        m_queue.close();
        if (m_thread.joinable()) m_thread.join();
    }

    std::size_t LogWorker::processed() const noexcept {
        return m_processed.load(std::memory_order_relaxed);
    }

    std::size_t LogWorker::failed() const noexcept {
        return m_failed.load(std::memory_order_relaxed);
    }

    void LogWorker::loop() {
        SMessage message;
        while (m_queue.pop(message)) {
            if (m_logger.log(message.level, message.message)) {
                m_processed.fetch_add(1, std::memory_order_relaxed);
            } else {
                m_failed.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }
} // namespace App
