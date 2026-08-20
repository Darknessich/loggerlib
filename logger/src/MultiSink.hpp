#pragma once

#include "ISink.hpp"

#include <memory>
#include <vector>

namespace Logger {
    // Writes a record to every sink
    class MultiSink final : public ISink {
    public:
        explicit MultiSink(std::vector<std::unique_ptr<ISink>> sinks) noexcept;

        bool writeLine(std::string_view line, std::error_code& ec) override;

    private:
        std::vector<std::unique_ptr<ISink>> m_sinks;
    };
} // namespace Logger
