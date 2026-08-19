#include "LogWorker.hpp"

#include <exception>
#include <system_error>
#include <utility>

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

    std::string LogWorker::lastError() const {
        const std::lock_guard lock(m_errorMutex);
        return m_lastError;
    }

    void LogWorker::recordFailure(std::string reason) {
        {
            const std::lock_guard lock(m_errorMutex);
            m_lastError = std::move(reason);
        }
        m_failed.fetch_add(1, std::memory_order_relaxed);
    }

    void LogWorker::loop() {
        SMessage message;
        while (m_queue.pop(message)) {
            std::error_code ec;
            bool written = false;

            try {
                written = m_logger.log(message.level, message.message, ec);
            } catch (const std::exception& error) {
                recordFailure(error.what());
                continue;
            } catch (...) {
                recordFailure("unknown exception");
                continue;
            }

            if (written) {
                m_processed.fetch_add(1, std::memory_order_relaxed);
            } else {
                recordFailure(ec ? ec.message() : "write failed");
            }
        }
    }
} // namespace App
