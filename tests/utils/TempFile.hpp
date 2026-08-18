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

        [[nodiscard]] const std::string& path() const noexcept { return m_path; }
        [[nodiscard]] bool exists() const;
        [[nodiscard]] std::vector<std::string> readLines() const;

    private:
        std::string m_path;
    };

} // namespace utils
