#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace Collector {
    // Cuts a stream of bytes into lines, one instance per connection
    class LineReader {
    public:
        static constexpr std::size_t kDefaultLimit = std::size_t{1024} * 1024;

        explicit LineReader(std::size_t limit = kDefaultLimit) noexcept;

        bool feed(std::string_view chunk, std::vector<std::string>& lines);

    private:
        std::size_t m_limit;
        std::string m_pending;
        bool m_skipping{false};
    };
} // namespace Collector
