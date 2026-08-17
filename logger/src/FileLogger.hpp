#pragma once

#include "LoggerBase.hpp"

#include <fstream>

namespace Logger {
    class FileLogger final : public LoggerBase {
    public:
        FileLogger(std::ofstream&& stream, ELogLevel level);

    protected:
        bool writeLine(std::string_view line) override;

    private:
        std::ofstream m_fstream;
    };
} // namespace Logger
