#pragma once

#include <string_view>
#include <system_error>

namespace Logger {
    class ISink {
    public:
        ISink() = default;
        virtual ~ISink() = default;

        ISink(const ISink&) = delete;
        ISink& operator=(const ISink&) = delete;
        ISink(ISink&&) = delete;
        ISink& operator=(ISink&&) = delete;

        [[nodiscard]] virtual bool writeLine(std::string_view line, std::error_code& ec) = 0;
    };
} // namespace Logger
