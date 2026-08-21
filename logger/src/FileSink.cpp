#include "FileSink.hpp"

#include <common/Errors.hpp>

#include <cerrno>
#include <utility>

namespace Logger {
    FileSink::FileSink(std::ofstream&& stream) : m_fstream{std::move(stream)} {}

    bool FileSink::writeLine(std::string_view line, std::error_code& ec) {
        m_fstream.clear();
        errno = 0;
        m_fstream << line << '\n' << std::flush;
        if (m_fstream.good()) return true;
        ec = Common::errnoError();
        return false;
    }

    std::unique_ptr<ISink> openFileSink(const std::string& path, std::error_code& ec) {
        errno = 0;
        std::ofstream stream{path, std::ios::out | std::ios::app};
        if (!stream.is_open()) {
            ec = Common::errnoError();
            return nullptr;
        }

        return std::make_unique<FileSink>(std::move(stream));
    }
} // namespace Logger
