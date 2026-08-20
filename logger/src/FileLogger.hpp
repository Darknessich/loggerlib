#pragma once

#include "LoggerBase.hpp"

#include <fstream>

namespace Logger {
    // Flushes every record
    class FileLogger final : public LoggerBase {
    public:
        FileLogger(std::ofstream&& stream, ELogLevel level);

    protected:
        bool writeLine(std::string_view line, std::error_code& ec) override;

    private:
        std::ofstream m_fstream;
    };
} // namespace Logger
