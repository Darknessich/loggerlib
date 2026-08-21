#include "LogWorker.hpp"

#include <common/Overloaded.hpp>

#include <exception>
#include <system_error>
#include <utility>
#include <variant>

namespace App {
    LogWorker::LogWorker(Logger::ILogger& logger, EventQueue& queue) noexcept
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

    void LogWorker::write(const SWrite& event) {
        // Without this check a message dropped by the threshold counts as processed
        if (!m_logger.isEnabled(event.level)) return;

        std::error_code ec;
        bool written = false;

        // An exception leaving a thread function is std::terminate
        try {
            written = m_logger.log(event.level, event.message, ec);
        } catch (const std::exception& error) {
            recordFailure(error.what());
            return;
        } catch (...) {
            recordFailure("unknown exception");
            return;
        }

        if (written) {
            m_processed.fetch_add(1, std::memory_order_relaxed);
        } else {
            recordFailure(ec ? ec.message() : "write failed");
        }
    }

    void LogWorker::loop() {
        TEvent event;
        while (m_queue.pop(event)) {
            std::visit(
                Common::SOverloaded{
                    [this](const SWrite& write) { this->write(write); },
                    [this](const SSetLevel& setLevel) { m_logger.setLevel(setLevel.level); }
                },
                event
            );
        }
    }
} // namespace App
