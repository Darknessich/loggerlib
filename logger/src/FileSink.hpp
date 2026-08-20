#pragma once

#include "ISink.hpp"

#include <fstream>
#include <memory>
#include <string>

namespace Logger {
    // Flushes every record
    class FileSink final : public ISink {
    public:
        explicit FileSink(std::ofstream&& stream);

        bool writeLine(std::string_view line, std::error_code& ec) override;

    private:
        std::ofstream m_fstream;
    };

    // Appends to @p path, creating it when missing
    std::unique_ptr<ISink> openFileSink(const std::string& path, std::error_code& ec);
} // namespace Logger
