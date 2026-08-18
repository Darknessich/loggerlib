#include <logger/LoggerFactory.hpp>

#include "FileLogger.hpp"

#include <cerrno>
#include <cstddef>
#include <fstream>
#include <utility>

namespace Logger {
    std::unique_ptr<ILogger>
    createFileLogger(const std::string& path, ELogLevel level, std::error_code& ec) {
        ec.clear();

        if (static_cast<std::size_t>(level) >= static_cast<std::size_t>(ELogLevel::Count)) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return nullptr;
        }

        errno = 0;
        std::ofstream stream{path, std::ios::out | std::ios::app};
        if (!stream.is_open()) {
            ec = errno != 0 ? std::error_code{errno, std::generic_category()}
                            : std::make_error_code(std::errc::io_error);
            return nullptr;
        }

        return std::make_unique<FileLogger>(std::move(stream), level);
    }
} // namespace Logger
