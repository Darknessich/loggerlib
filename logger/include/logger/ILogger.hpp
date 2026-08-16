#pragma once

#include <logger/Export.h>
#include <logger/LogLevel.hpp>

#include <string_view>

namespace Logger {
    class LOGGER_EXPORT ILogger {
    public:
        virtual ~ILogger() = default;
        virtual bool log(ELogLevel level, std::string_view message) = 0;
        virtual void setLevel(ELogLevel level) noexcept = 0;
        virtual ELogLevel level() const noexcept = 0;

        bool isEnabled(ELogLevel level) const noexcept {
            return level >= this->level();
        }
    };
} // namespace Logger
