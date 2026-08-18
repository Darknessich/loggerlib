#pragma once

#include <logger/ILogger.hpp>
#include <logger/LogLevel.hpp>

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace utils {
    class RecordingLogger : public Logger::ILogger {
    public:
        struct SRecord {
            Logger::ELogLevel level{Logger::ELogLevel::Info};
            std::string message;
        };

        explicit RecordingLogger(Logger::ELogLevel level) noexcept : m_level{level} {}

        bool log(Logger::ELogLevel level, std::string_view message) override {
            if (!isEnabled(level)) return true;

            const std::lock_guard lock(m_mutex);
            m_records.push_back({level, std::string{message}});
            return m_succeeds.load(std::memory_order_relaxed);
        }

        void setLevel(Logger::ELogLevel level) noexcept override {
            m_level.store(level, std::memory_order_relaxed);
        }

        Logger::ELogLevel level() const noexcept override {
            return m_level.load(std::memory_order_relaxed);
        }

        void failWrites() noexcept { m_succeeds.store(false, std::memory_order_relaxed); }

        std::vector<SRecord> records() const {
            const std::lock_guard lock(m_mutex);
            return m_records;
        }

        std::size_t count() const {
            const std::lock_guard lock(m_mutex);
            return m_records.size();
        }

    private:
        std::atomic<Logger::ELogLevel> m_level;
        std::atomic<bool> m_succeeds{true};
        mutable std::mutex m_mutex;
        std::vector<SRecord> m_records;
    };
} // namespace utils
