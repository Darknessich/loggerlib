#include "FileLogger.hpp"

#include <cerrno>
#include <utility>

namespace Logger {
    FileLogger::FileLogger(std::ofstream&& stream, ELogLevel level)
        : LoggerBase(level), m_fstream{std::move(stream)} {}

    bool FileLogger::writeLine(std::string_view line, std::error_code& ec) {
        m_fstream.clear();
        errno = 0;
        m_fstream << line << '\n' << std::flush;
        if (m_fstream.good()) return true;
        ec = errno != 0 ? std::error_code{errno, std::generic_category()}
                        : std::make_error_code(std::errc::io_error);
        return false;
    }
} // namespace Logger
