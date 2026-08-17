#pragma once

#include <string>
#include <vector>

namespace utils {
    class TempFile {
    public:
        explicit TempFile(std::string name);
        ~TempFile();

        TempFile(const TempFile&) = delete;
        TempFile& operator=(const TempFile&) = delete;
        TempFile(TempFile&&) = delete;
        TempFile& operator=(TempFile&&) = delete;

        const std::string& path() const noexcept { return m_path; }
        bool exists() const;
        std::vector<std::string> readLines() const;

    private:
        std::string m_path;
    };

} // namespace utils
