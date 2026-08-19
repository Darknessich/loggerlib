#pragma once

#include <logger/Export.h>
#include <logger/LogLevel.hpp>

#include <string_view>
#include <system_error>

namespace Logger {
    class LOGGER_EXPORT ILogger {
    public:
        virtual ~ILogger() = default;

        [[nodiscard]] virtual bool
        log(ELogLevel level, std::string_view message, std::error_code& ec) = 0;
        virtual void setLevel(ELogLevel level) noexcept = 0;
        [[nodiscard]] virtual ELogLevel level() const noexcept = 0;

        [[nodiscard]] bool log(ELogLevel level, std::string_view message) {
            std::error_code ignored;
            return this->log(level, message, ignored);
        }

        [[nodiscard]] bool isEnabled(ELogLevel level) const noexcept {
            return level >= this->level();
        }

    protected:
        ILogger() = default;
        ILogger(const ILogger&) = default;
        ILogger& operator=(const ILogger&) = default;
        ILogger(ILogger&&) = default;
        ILogger& operator=(ILogger&&) = default;
    };
} // namespace Logger
