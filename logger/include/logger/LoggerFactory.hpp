#pragma once

#include <logger/Export.h>
#include <logger/ILogger.hpp>
#include <logger/LogLevel.hpp>

#include <memory>
#include <string>
#include <system_error>

namespace Logger {
    LOGGER_EXPORT std::unique_ptr<ILogger>
    createFileLogger(const std::string& path,
                     ELogLevel level,
                     std::error_code& ec);
} // namespace Logger
