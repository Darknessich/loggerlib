#pragma once

#include <logger/ILogger.hpp>
#include <logger/LogLevel.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <string_view>

namespace Logger {
    // Threshold, filtering, timestamp and serialisation of writers
    class LoggerBase : public ILogger {
    public:
        using ILogger::log;

        [[nodiscard]] bool
        log(ELogLevel level, std::string_view message, std::error_code& ec) final;
        void setLevel(ELogLevel level) noexcept final;
        [[nodiscard]] ELogLevel level() const noexcept final;

    protected:
        explicit LoggerBase(ELogLevel level) noexcept;

        [[nodiscard]] virtual bool writeLine(std::string_view line, std::error_code& ec) = 0;

        // Virtual for fixed time in tests
        [[nodiscard]] virtual std::chrono::system_clock::time_point now() const noexcept;

    private:
        std::atomic<ELogLevel> m_level;
        std::mutex m_mutex;
    };
} // namespace Logger
