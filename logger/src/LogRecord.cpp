#include <logger/LogRecord.hpp>

#include <array>
#include <charconv>
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <ctime>

namespace {
    constexpr std::size_t kStampSize = 23; // "YYYY-MM-DD HH:MM:SS.mmm"
    constexpr char kUnknownStamp[] = "0000-00-00 00:00:00.000";
    static_assert(sizeof(kUnknownStamp) == kStampSize + 1, "fallback stamp must be as wide as a real one");

    [[nodiscard]] bool parseInt(std::string_view text, int& out) noexcept {
        const char* const first = text.data();
        const char* const last  = text.data() + text.size();
        const auto result = std::from_chars(first, last, out);
        return result.ec == std::errc{} && result.ptr == last;
    }

    bool isLeapYear(int year) noexcept {
        return year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
    }

    bool isCorrectDate(int year, int month, int day) noexcept {
        static constexpr std::array<int, 12> daysInMonth{{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};
        return month >= 1 && month <= 12  && day >= 1
            && (month == 2 && isLeapYear(year) ? day <= 29 : day <= daysInMonth[month - 1]);
    }

    std::optional<std::chrono::system_clock::time_point> parseTimestamp(std::string_view line) noexcept {
        if (line.size() < kStampSize) return std::nullopt;

        if (line[4] != '-' || line[7] != '-' || line[10] != ' '
            || line[13] != ':' || line[16] != ':' || line[19] != '.') return std::nullopt;

        int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, millis = 0;
        if (!parseInt(line.substr(0, 4),  year)   || !parseInt(line.substr(5, 2),  month) ||
            !parseInt(line.substr(8, 2),  day)    || !parseInt(line.substr(11, 2), hour)  ||
            !parseInt(line.substr(14, 2), minute) || !parseInt(line.substr(17, 2), second)||
            !parseInt(line.substr(20, 3), millis)) return std::nullopt;


        if (!isCorrectDate(year, month, day) ||
            hour   < 0 || hour   > 23 || minute < 0 || minute > 59 ||
            second < 0 || second > 59 || millis < 0 || millis > 999) return std::nullopt;

        std::tm parts{};
        parts.tm_year = year - 1900; parts.tm_mon = month - 1; parts.tm_mday = day;
        parts.tm_hour = hour; parts.tm_min = minute; parts.tm_sec = second;
        parts.tm_isdst = -1;

        const std::time_t raw = std::mktime(&parts);
        return raw == static_cast<std::time_t>(-1)
               ? std::nullopt
               : std::optional{
                    std::chrono::system_clock::from_time_t(raw)
                    + std::chrono::milliseconds{millis}};
    }
} // namespace

namespace Logger {
    std::string formatRecord(const SLogRecord& record) {
        return formatRecord(record.time, record.level, record.message);
    }

    std::string formatRecord(std::chrono::system_clock::time_point time,
                             ELogLevel level, std::string_view message) {
        const auto seconds = std::chrono::floor<std::chrono::seconds>(time);
        const auto millis  = std::chrono::duration_cast<std::chrono::milliseconds>(time - seconds).count();
        const std::time_t raw = std::chrono::system_clock::to_time_t(seconds);

        std::tm parts{};
        char stamp[kStampSize + 1] = {};

        const int written = ::localtime_r(&raw, &parts) != nullptr
            ? std::snprintf(stamp, sizeof(stamp), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                            parts.tm_year + 1900, parts.tm_mon + 1, parts.tm_mday,
                            parts.tm_hour, parts.tm_min, parts.tm_sec, static_cast<int>(millis))
            : -1;

        if (written != static_cast<int>(kStampSize)) {
            std::memcpy(stamp, kUnknownStamp, sizeof(kUnknownStamp));
        }

        const std::string_view name = level2string(level);
        const std::string escaped = escapeMessage(message);

        std::string out;
        out.reserve(kStampSize + name.size() + escaped.size() + 4);
        out += stamp; out += " ["; out += name; out += "] "; out += escaped;
        return out;
    }

    std::optional<SLogRecord> parseRecord(std::string_view line) {
        if (line.size() < kStampSize + 4) return std::nullopt;
        const auto time = parseTimestamp(line);
        if (!time) return std::nullopt;

        const std::string_view rest = line.substr(kStampSize);
        if (rest.size() < 3 || rest[0] != ' ' || rest[1] != '[') return std::nullopt;
        const auto close = rest.find(']', 2);
        if (close == std::string_view::npos || close + 1 >= rest.size() || rest[close + 1] != ' ')
            return std::nullopt;

        const auto level = string2level(rest.substr(2, close - 2));
        if (!level) return std::nullopt;

        return SLogRecord{
            *time,
            *level,
            unescapeMessage(rest.substr(close + 2))
        };
    }

    std::string escapeMessage(std::string_view message) {
        std::string out;
        out.reserve(message.size());
        for (auto c: message) {
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                default: out += c; break;
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
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case '\\': out += '\\'; break;
                default: out += '\\'; out += message[i]; break;
            }
        }
        return out;
    }
} // namespace Logger
