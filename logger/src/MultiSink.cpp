#include "MultiSink.hpp"

#include <utility>

namespace Logger {
    MultiSink::MultiSink(std::vector<std::unique_ptr<ISink>> sinks) noexcept
        : m_sinks{std::move(sinks)} {}

    bool MultiSink::writeLine(std::string_view line, std::error_code& ec) {
        bool written = true;

        for (const auto& sink : m_sinks) {
            std::error_code sinkError;
            if (sink->writeLine(line, sinkError)) continue;

            if (written) ec = sinkError;
            written = false;
        }

        return written;
    }
} // namespace Logger
