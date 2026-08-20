#pragma once

#include "ISink.hpp"

#include <logger/ILogger.hpp>
#include <logger/LogLevel.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string_view>
#include <system_error>

namespace Logger {
    // Threshold, filtering, timestamp and serialisation of writers
    class SinkLogger : public ILogger {
    public:
        using ILogger::log;

        SinkLogger(std::unique_ptr<ISink> sink, ELogLevel level) noexcept;

        [[nodiscard]] bool
        log(ELogLevel level, std::string_view message, std::error_code& ec) final;
        void setLevel(ELogLevel level) noexcept final;
        [[nodiscard]] ELogLevel level() const noexcept final;

    protected:
        // virtual for fixed time in tests
        [[nodiscard]] virtual std::chrono::system_clock::time_point now() const noexcept;

    private:
        std::unique_ptr<ISink> m_sink;
        std::atomic<ELogLevel> m_level;
        std::mutex m_mutex;
    };
} // namespace Logger
