#include "FileLogger.hpp"

#include <utility>

namespace Logger {
    FileLogger::FileLogger(std::ofstream&& stream, ELogLevel level)
        : LoggerBase(level), m_fstream{std::move(stream)}
    {}

    bool FileLogger::writeLine(std::string_view line) {
        m_fstream << line << '\n' << std::flush;
        return m_fstream.good();
    }
} // namespace Logger
