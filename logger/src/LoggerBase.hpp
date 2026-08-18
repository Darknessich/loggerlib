#pragma once

#include <logger/ILogger.hpp>
#include <logger/LogLevel.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <string_view>

namespace Logger {
    class LoggerBase : public ILogger {
    public:
        [[nodiscard]] bool log(ELogLevel level, std::string_view message) final;
        void setLevel(ELogLevel level) noexcept final;
        [[nodiscard]] ELogLevel level() const noexcept final;

    protected:
        explicit LoggerBase(ELogLevel level) noexcept;

        [[nodiscard]] virtual bool writeLine(std::string_view line) = 0;

        [[nodiscard]] virtual std::chrono::system_clock::time_point now() const noexcept;

    private:
        std::atomic<ELogLevel> m_level;
        std::mutex m_mutex;
    };
} // namespace Logger
