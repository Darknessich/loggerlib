#include "LoggerBase.hpp"

#include <logger/LogRecord.hpp>

#include <chrono>
#include <string>

namespace Logger {
    LoggerBase::LoggerBase(ELogLevel level) noexcept
        : m_level{level}
    {}

    bool LoggerBase::log(ELogLevel level, std::string_view message) {
        if (!isEnabled(level)) return true;
        const std::string line = formatRecord(now(), level, message);
        std::lock_guard lock(m_mutex);
        return writeLine(line);
    }

    void LoggerBase::setLevel(ELogLevel level) noexcept {
        m_level.store(level, std::memory_order_relaxed);
    }

    ELogLevel LoggerBase::level() const noexcept {
        return m_level.load(std::memory_order_relaxed);
    }

    std::chrono::system_clock::time_point LoggerBase::now() const noexcept {
        return std::chrono::system_clock::now();
    }
} // namespace Logger
