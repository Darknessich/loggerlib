#include <utils/TempFile.hpp>

#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>

namespace utils {
    namespace {
        void removeQuietly(const std::string& path) {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
    } // namespace

    TempFile::TempFile(std::string name) : m_path{std::move(name)} {
        removeQuietly(m_path);
    }

    TempFile::~TempFile() {
        removeQuietly(m_path);
    }

    bool TempFile::exists() const {
        std::error_code ec;
        return std::filesystem::exists(m_path, ec);
    }

    std::vector<std::string> TempFile::readLines() const {
        std::vector<std::string> lines;

        std::ifstream input{m_path};
        for (std::string line; std::getline(input, line); ) {
            lines.push_back(std::move(line));
        }
        return lines;
    }

} // namespace utils
