#include "LineReader.hpp"

#include <utility>

namespace Collector {
    LineReader::LineReader(std::size_t limit) noexcept : m_limit{limit} {}

    bool LineReader::feed(std::string_view chunk, std::vector<std::string>& lines) {
        bool complete = true;

        while (!chunk.empty()) {
            const auto end = chunk.find('\n');

            if (m_skipping) {
                if (end == std::string_view::npos) break;

                chunk.remove_prefix(end + 1);
                m_skipping = false;
                continue;
            }

            if (end == std::string_view::npos) {
                if (m_pending.size() + chunk.size() > m_limit) {
                    m_pending.clear();
                    m_skipping = true;
                    return false;
                }

                m_pending.append(chunk);
                break;
            }

            if (m_pending.size() + end > m_limit) {
                m_pending.clear();
                complete = false;
            } else {
                m_pending.append(chunk.substr(0, end));
                lines.push_back(std::move(m_pending));
                m_pending.clear();
            }

            chunk.remove_prefix(end + 1);
        }

        return complete;
    }
} // namespace Collector
