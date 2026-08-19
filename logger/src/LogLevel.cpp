#include <logger/LogLevel.hpp>

#include <array>
#include <cstddef>

namespace Logger {
    namespace {
        struct SLevelName {
            ELogLevel level;
            std::string_view name;
        };

        inline constexpr std::size_t kLevelCount = static_cast<std::size_t>(ELogLevel::Count);
        inline constexpr std::array<SLevelName, kLevelCount> kLevelNames{
            {{ELogLevel::Debug, "DEBUG"},
             {ELogLevel::Info, "INFO"},
             {ELogLevel::Warn, "WARN"},
             {ELogLevel::Error, "ERROR"},
             {ELogLevel::Fatal, "FATAL"}}
        };

        constexpr bool isTableOrdered() noexcept {
            for (std::size_t i = 0; i < kLevelCount; ++i) {
                if (static_cast<std::size_t>(kLevelNames[i].level) != i) {
                    return false;
                }
            }
            return true;
        }

        static_assert(isTableOrdered(), "kLevelNames must be ordered by enum value");

        char toLowerCase(char c) noexcept {
            return 'A' <= c && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
        }

        bool equalsIgnoreCase(std::string_view lhs, std::string_view rhs) noexcept {
            if (lhs.size() != rhs.size()) return false;

            for (std::size_t i = 0; i < lhs.size(); ++i) {
                if (toLowerCase(lhs[i]) != toLowerCase(rhs[i])) {
                    return false;
                }
            }
            return true;
        }
    } // namespace

    std::string_view level2string(ELogLevel level) noexcept {
        const std::size_t index = static_cast<std::size_t>(level);
        return isValidLevel(level) ? kLevelNames[index].name : std::string_view{"UNKNOWN"};
    }

    std::optional<ELogLevel> string2level(std::string_view str) noexcept {
        for (const auto& entry : kLevelNames) {
            if (equalsIgnoreCase(entry.name, str)) {
                return entry.level;
            }
        }
        return std::nullopt;
    }

    bool isValidLevel(ELogLevel level) noexcept {
        return level < ELogLevel::Count;
    }
} // namespace Logger
