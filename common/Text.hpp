#pragma once

#include <charconv>
#include <cstddef>
#include <optional>
#include <string_view>
#include <system_error>

namespace Common {
    /// @brief Lower-cases one ASCII letter.
    /// @param c character to fold
    /// @return the lower-case letter, or @p c unchanged
    /// @note Not std::tolower, e.g. in a Turkish locale 'I' maps to a dotless 'ı'
    constexpr char toLowerCase(char c) noexcept {
        return 'A' <= c && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
    }

    /// @brief Compares two strings ignoring the case of ASCII letters.
    /// @param lhs first string
    /// @param rhs second string
    /// @return true when both hold the same characters
    constexpr bool equalsIgnoreCase(std::string_view lhs, std::string_view rhs) noexcept {
        if (lhs.size() != rhs.size()) return false;

        for (std::size_t i = 0; i < lhs.size(); ++i) {
            if (toLowerCase(lhs[i]) != toLowerCase(rhs[i])) return false;
        }
        return true;
    }

    /// @brief Returns true when @p c is a decimal digit.
    /// @param c character to test
    /// @return true for '0' to '9'
    /// @note Not std::isdigit: undefined on a negative char, and locale-dependent
    constexpr bool isDigit(char c) noexcept {
        return '0' <= c && c <= '9';
    }

    /// @brief Reads a number, the whole text or nothing.
    /// @tparam T   any type std::from_chars reads
    /// @param text text to read, a leading space or a tail makes the call fail
    /// @return the number or std::nullopt
    template <typename T> std::optional<T> parseNumber(std::string_view text) noexcept {
        const char* const first = text.data();
        const char* const last = first + text.size();

        T value{};
        const auto result = std::from_chars(first, last, value);
        if (result.ec != std::errc{} || result.ptr != last) return std::nullopt;
        return value;
    }
} // namespace Common
