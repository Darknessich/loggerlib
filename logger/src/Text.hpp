#pragma once

#include <cstddef>
#include <string_view>

namespace Logger {
    // Not std::tolower, e.g. in a Turkish locale 'I' maps to a dotless 'ı'
    constexpr char toLowerCase(char c) noexcept {
        return 'A' <= c && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
    }

    constexpr bool equalsIgnoreCase(std::string_view lhs, std::string_view rhs) noexcept {
        if (lhs.size() != rhs.size()) return false;

        for (std::size_t i = 0; i < lhs.size(); ++i) {
            if (toLowerCase(lhs[i]) != toLowerCase(rhs[i])) return false;
        }
        return true;
    }
} // namespace Logger
