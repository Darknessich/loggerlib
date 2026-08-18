#include <logger/LogRecord.hpp>

#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace {
    constexpr char kUnknownStamp[] = "0000-00-00 00:00:00.000Z";
    constexpr char kFormatString[] = "%04d-%02d-%02d %02d:%02d:%02d.%03dZ";
    constexpr std::size_t kStampSize = sizeof(kUnknownStamp) - 1;

    bool parseInt(std::string_view text, int& out) noexcept {
        const char* const first = text.data();
        const char* const last = text.data() + text.size();
        const auto result = std::from_chars(first, last, out);
        return result.ec == std::errc{} && result.ptr == last;
    }

    constexpr bool isDigit(char c) noexcept {
        return '0' <= c && c <= '9';
    }

    std::optional<std::chrono::system_clock::time_point> parseTimestamp(std::string_view line) {
        if (line.size() != kStampSize) return std::nullopt;

        for (std::size_t i = 0; i < kStampSize; ++i) {
            const bool ok =
                isDigit(kUnknownStamp[i]) ? isDigit(line[i]) : kUnknownStamp[i] == line[i];
            if (!ok) return std::nullopt;
        }

        std::tm parts{};
        int year{0}, month{0}, millis{0};
        if (!parseInt(line.substr(0, 4), year) || !parseInt(line.substr(5, 2), month) ||
            !parseInt(line.substr(8, 2), parts.tm_mday) ||
            !parseInt(line.substr(11, 2), parts.tm_hour) ||
            !parseInt(line.substr(14, 2), parts.tm_min) ||
            !parseInt(line.substr(17, 2), parts.tm_sec) || !parseInt(line.substr(20, 3), millis))
            return std::nullopt;

        parts.tm_year = year - 1900;
        parts.tm_mon = month - 1;

        errno = 0;
        const auto requested = parts;
        const std::time_t raw = ::timegm(&parts);
        if (raw == static_cast<std::time_t>(-1) && errno != 0) return std::nullopt;

        if (parts.tm_year != requested.tm_year || parts.tm_mon != requested.tm_mon ||
            parts.tm_mday != requested.tm_mday || parts.tm_hour != requested.tm_hour ||
            parts.tm_min != requested.tm_min || parts.tm_sec != requested.tm_sec)
            return std::nullopt;

        return std::optional{
            std::chrono::system_clock::from_time_t(raw) + std::chrono::milliseconds{millis}
        };
    }
} // namespace

namespace Logger {
    std::string formatRecord(const SLogRecord& record) {
        return formatRecord(record.time, record.level, record.message);
    }

    std::string formatRecord(
        std::chrono::system_clock::time_point time, ELogLevel level, std::string_view message
    ) {
        const auto seconds = std::chrono::floor<std::chrono::seconds>(time);
        const auto millis =
            std::chrono::duration_cast<std::chrono::milliseconds>(time - seconds).count();
        const std::time_t raw = std::chrono::system_clock::to_time_t(seconds);

        std::tm parts{};
        char stamp[kStampSize + 1] = {};

        int written{-1};
        if (::gmtime_r(&raw, &parts) != nullptr) {
            written = std::snprintf(
                stamp,
                sizeof(stamp),
                kFormatString,
                parts.tm_year + 1900,
                parts.tm_mon + 1,
                parts.tm_mday,
                parts.tm_hour,
                parts.tm_min,
                parts.tm_sec,
                static_cast<int>(millis)
            );
        }

        if (written != static_cast<int>(kStampSize)) {
            std::memcpy(stamp, kUnknownStamp, sizeof(kUnknownStamp));
        }

        const std::string_view name = level2string(level);
        const std::string escaped = escapeMessage(message);

        std::string out;
        out.reserve(kStampSize + name.size() + escaped.size() + 4);
        out += stamp;
        out += " [";
        out += name;
        out += "] ";
        out += escaped;
        return out;
    }

    std::optional<SLogRecord> parseRecord(std::string_view line) {
        if (line.size() < kStampSize) return std::nullopt;
        const auto time = parseTimestamp(line.substr(0, kStampSize));
        if (!time) return std::nullopt;

        const std::string_view rest = line.substr(kStampSize);
        if (rest.size() < 3 || rest[0] != ' ' || rest[1] != '[') return std::nullopt;
        const auto close = rest.find(']', 2);
        if (close == std::string_view::npos || close + 1 >= rest.size() || rest[close + 1] != ' ')
            return std::nullopt;

        const auto level = string2level(rest.substr(2, close - 2));
        if (!level) return std::nullopt;

        return SLogRecord{*time, *level, unescapeMessage(rest.substr(close + 2))};
    }

    std::string escapeMessage(std::string_view message) {
        std::string out;
        out.reserve(message.size());
        for (auto c : message) {
            switch (c) {
                case '\\':
                    out += "\\\\";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                default:
                    out += c;
                    break;
            }
        }
        return out;
    }

    std::string unescapeMessage(std::string_view message) {
        std::string out;
        out.reserve(message.size());
        for (std::size_t i = 0; i < message.size(); ++i) {
            if (message[i] != '\\' || i + 1 == message.size()) {
                out += message[i];
                continue;
            }
            switch (message[++i]) {
                case 'n':
                    out += '\n';
                    break;
                case 'r':
                    out += '\r';
                    break;
                case '\\':
                    out += '\\';
                    break;
                default:
                    out += '\\';
                    out += message[i];
                    break;
            }
        }
        return out;
    }
} // namespace Logger
