#pragma once

#include <logger/Export.h>
#include <logger/LogLevel.hpp>

#include <string_view>
#include <system_error>

namespace Logger {
    /// @brief Interface every logger of this library implements.
    class LOGGER_EXPORT ILogger {
    public:
        virtual ~ILogger() = default;

        /// @brief Logs a message.
        /// @param level    severity of the message
        /// @param message  text of the message
        /// @param ec       failure reason
        /// @return false on failure
        [[nodiscard]] virtual bool
        log(ELogLevel level, std::string_view message, std::error_code& ec) = 0;

        /// @brief Replaces the threshold.
        /// @param level new threshold
        virtual void setLevel(ELogLevel level) noexcept = 0;

        /// @brief Returns the current threshold.
        [[nodiscard]] virtual ELogLevel level() const noexcept = 0;

        /// @brief Logs a message, discarding the reason if it fails.
        /// @param level    severity of the message
        /// @param message  text of the message
        /// @return the same value the three-argument overload would return
        [[nodiscard]] bool log(ELogLevel level, std::string_view message) {
            std::error_code ignored;
            return this->log(level, message, ignored);
        }

        /// @brief Returns true when @p level is not below the current threshold.
        [[nodiscard]] bool isEnabled(ELogLevel level) const noexcept {
            return level >= this->level();
        }

    protected:
        /// @cond
        ILogger() = default;
        ILogger(const ILogger&) = default;
        ILogger& operator=(const ILogger&) = default;
        ILogger(ILogger&&) = default;
        ILogger& operator=(ILogger&&) = default;
        /// @endcond
    };
} // namespace Logger
