#include "SinkLogger.hpp"

#include <logger/LogRecord.hpp>

#include <string>
#include <utility>

namespace Logger {
    SinkLogger::SinkLogger(std::unique_ptr<ISink> sink, ELogLevel level) noexcept
        : m_sink{std::move(sink)}, m_level{level} {}

    bool SinkLogger::log(ELogLevel level, std::string_view message, std::error_code& ec) {
        ec.clear();
        if (!isValidLevel(level)) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        // Dropped by the threshold is a success with a clean ec, not a failure
        if (!isEnabled(level)) return true;

        const std::string line = formatRecord(now(), level, message);
        const std::lock_guard lock(m_mutex);
        return m_sink->writeLine(line, ec);
    }

    void SinkLogger::setLevel(ELogLevel level) noexcept {
        m_level.store(level, std::memory_order_relaxed);
    }

    ELogLevel SinkLogger::level() const noexcept {
        return m_level.load(std::memory_order_relaxed);
    }

    std::chrono::system_clock::time_point SinkLogger::now() const noexcept {
        return std::chrono::system_clock::now();
    }
} // namespace Logger
