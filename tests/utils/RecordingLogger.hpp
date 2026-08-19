#pragma once

#include <logger/ILogger.hpp>
#include <logger/LogLevel.hpp>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace utils {
    class RecordingLogger : public Logger::ILogger {
    public:
        struct SRecord {
            Logger::ELogLevel level{Logger::ELogLevel::Info};
            std::string message;
        };

        explicit RecordingLogger(Logger::ELogLevel level) noexcept : m_level{level} {}

        using Logger::ILogger::log;

        bool log(Logger::ELogLevel level, std::string_view message, std::error_code& ec) override {
            ec.clear();
            if (!isEnabled(level)) return true;

            const std::lock_guard lock(m_mutex);
            if (m_throws) throw std::runtime_error{"write exploded"};

            m_records.push_back({level, std::string{message}});
            if (m_errors.empty()) return true;

            ec = m_errors[std::min(m_failures, m_errors.size() - 1)];
            ++m_failures;
            m_failureAdded.notify_all();
            return false;
        }

        void waitForFailures(std::size_t count) const {
            std::unique_lock lock(m_mutex);
            m_failureAdded.wait(lock, [this, count] { return m_failures >= count; });
        }

        void setLevel(Logger::ELogLevel level) noexcept override {
            m_level.store(level, std::memory_order_relaxed);
        }

        Logger::ELogLevel level() const noexcept override {
            return m_level.load(std::memory_order_relaxed);
        }

        void failWrites(std::error_code ec = std::make_error_code(std::errc::io_error)) {
            failWrites(std::vector<std::error_code>{ec});
        }

        void failWrites(std::vector<std::error_code> errors) {
            const std::lock_guard lock(m_mutex);
            m_errors = std::move(errors);
            m_failures = 0;
        }

        void throwOnWrite() {
            const std::lock_guard lock(m_mutex);
            m_throws = true;
        }

        [[nodiscard]] std::vector<SRecord> records() const {
            const std::lock_guard lock(m_mutex);
            return m_records;
        }

        [[nodiscard]] std::size_t count() const {
            const std::lock_guard lock(m_mutex);
            return m_records.size();
        }

    private:
        std::atomic<Logger::ELogLevel> m_level;
        mutable std::mutex m_mutex;
        mutable std::condition_variable m_failureAdded;
        std::vector<std::error_code> m_errors;
        std::size_t m_failures = 0;
        bool m_throws = false;
        std::vector<SRecord> m_records;
    };
} // namespace utils
