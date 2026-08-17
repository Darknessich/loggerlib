#include "FileLogger.hpp"

#include <utility>

namespace Logger {
    FileLogger::FileLogger(std::ofstream&& stream, ELogLevel level)
        : LoggerBase(level), m_fstream{std::move(stream)}
    {}

    bool FileLogger::writeLine(std::string_view line) {
        m_fstream << line << '\n';
        m_fstream.flush();
        return m_fstream.good();
    }
} // namespace Logger
