#include <framework/TestFramework.hpp>

#include <logger/LogLevel.hpp>
#include <logger/LogRecord.hpp>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <initializer_list>
#include <string>

using Logger::ELogLevel;
using Logger::SLogRecord;

namespace {
    [[maybe_unused]] const bool kTimeZoneFixed = [] {
        ::setenv("TZ", "UTC", 1);
        ::tzset();
        return true;
    }();

    std::chrono::system_clock::time_point instant(std::time_t seconds, int millis = 0) {
        return std::chrono::system_clock::from_time_t(seconds) + std::chrono::milliseconds{millis};
    }

    long long millisSinceEpoch(std::chrono::system_clock::time_point time) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count();
    }

} // namespace

TEST(log_record, formats_known_instants) {
    CHECK_EQ(Logger::formatRecord({instant(0, 123), ELogLevel::INFO, "hello"}),
             "1970-01-01 00:00:00.123 [INFO] hello");

    CHECK_EQ(Logger::formatRecord({instant(1'000'000'000), ELogLevel::FATAL, "boom"}),
             "2001-09-09 01:46:40.000 [FATAL] boom");
}

TEST(log_record, formats_empty_message) {
    CHECK_EQ(Logger::formatRecord({instant(0), ELogLevel::WARN, ""}),
             "1970-01-01 00:00:00.000 [WARN] ");
}

TEST(log_record, round_trips_through_format_and_parse) {
    for (const std::string message : {"", "simple", "with  spaces", "юникод",
                                      "a\nb", "a\rb", "back\\slash", "]", "[INFO] fake",
                                      "trailing backslash \\"}) {
        const SLogRecord original{instant(1'000'000'000, 456), ELogLevel::WARN, message};

        const auto parsed = Logger::parseRecord(Logger::formatRecord(original));
        if (!CHECK(parsed.has_value())) continue;

        CHECK_EQ(millisSinceEpoch(parsed->time), millisSinceEpoch(original.time));
        CHECK_EQ(parsed->level, original.level);
        CHECK_EQ(parsed->message, original.message);
    }
}

TEST(log_record, round_trips_every_level) {
    for (std::size_t index = 0; index < static_cast<std::size_t>(ELogLevel::COUNT); ++index) {
        const SLogRecord original{instant(0), static_cast<ELogLevel>(index), "text"};

        const auto parsed = Logger::parseRecord(Logger::formatRecord(original));
        if (!CHECK(parsed.has_value())) continue;
        CHECK_EQ(parsed->level, original.level);
    }
}

TEST(log_record, escaping_is_reversible) {
    CHECK_EQ(Logger::escapeMessage(""), "");
    CHECK_EQ(Logger::escapeMessage("plain"), "plain");
    CHECK_EQ(Logger::escapeMessage("a\nb"), "a\\nb");
    CHECK_EQ(Logger::escapeMessage("a\rb"), "a\\rb");
    CHECK_EQ(Logger::escapeMessage("a\\b"), "a\\\\b");
    CHECK_EQ(Logger::escapeMessage("\t"), "\t");

    CHECK(Logger::escapeMessage("a\nb") != Logger::escapeMessage("a\\nb"));

    for (const std::string text : {"", "plain", "a\nb", "a\rb", "a\\b", "\\", "\\\\",
                                   "\\n", "смешанный \\ текст\n"}) {
        CHECK_EQ(Logger::unescapeMessage(Logger::escapeMessage(text)), text);
    }
}

TEST(log_record, unescaping_tolerates_malformed_input) {
    CHECK_EQ(Logger::unescapeMessage("a\\qb"), "a\\qb");
    CHECK_EQ(Logger::unescapeMessage("a\\"),   "a\\");
    CHECK_EQ(Logger::unescapeMessage("\\"),    "\\");
    CHECK_EQ(Logger::unescapeMessage(""),      "");
}

TEST(log_record, rejects_short_and_truncated_lines) {
    CHECK(!Logger::parseRecord("").has_value());
    CHECK(!Logger::parseRecord("x").has_value());
    CHECK(!Logger::parseRecord(std::string(22, 'x')).has_value());
    CHECK(!Logger::parseRecord(std::string(23, 'x')).has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.000").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.000 [INFO]").has_value());
}

TEST(log_record, rejects_malformed_structure) {
    CHECK(!Logger::parseRecord("1970-01-01T00:00:00.000 [INFO] text").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00,000 [INFO] text").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.000 INFO text").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.000 [INFO]text").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.000 [NOPE] text").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.000 [] text").has_value());
    CHECK(!Logger::parseRecord("19x0-01-01 00:00:00.000 [INFO] text").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.00a [INFO] text").has_value());
}

TEST(log_record, rejects_impossible_dates) {
    CHECK(!Logger::parseRecord("1970-13-01 00:00:00.000 [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1970-00-01 00:00:00.000 [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1970-01-00 00:00:00.000 [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1970-01-32 00:00:00.000 [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1970-04-31 00:00:00.000 [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 24:00:00.000 [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:60:00.000 [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 23:59:60.000 [INFO] x").has_value());
}

TEST(log_record, handles_leap_years) {
    CHECK(Logger::parseRecord("2024-02-29 00:00:00.000 [INFO] x").has_value());
    CHECK(Logger::parseRecord("2000-02-29 00:00:00.000 [INFO] x").has_value());
    CHECK(!Logger::parseRecord("2023-02-29 00:00:00.000 [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1900-02-29 00:00:00.000 [INFO] x").has_value());
}

TEST(log_record, accepts_boundary_values) {
    CHECK(Logger::parseRecord("1970-01-01 23:59:59.999 [INFO] x").has_value());
    CHECK(Logger::parseRecord("1970-01-31 00:00:00.000 [INFO] x").has_value());
    CHECK(Logger::parseRecord("1970-12-31 00:00:00.000 [INFO] x").has_value());
    CHECK(Logger::parseRecord("1970-01-01 00:00:00.999 [INFO] x").has_value());
}

TEST(log_record, invalid_level_produces_unparseable_line) {
    const auto line = Logger::formatRecord({instant(0), static_cast<ELogLevel>(42), "x"});
    CHECK_EQ(line, "1970-01-01 00:00:00.000 [UNKNOWN] x");
    CHECK(!Logger::parseRecord(line).has_value());
}

TEST(log_record, both_format_overloads_agree) {
    const SLogRecord record{instant(1'000'000'000, 456), ELogLevel::WARN, "a\nb"};
    CHECK_EQ(Logger::formatRecord(record),
             Logger::formatRecord(record.time, record.level, record.message));
}

TEST(log_record, formats_a_view_without_a_terminator) {
    const char raw[] = {'H', 'E', 'L', 'L', 'O'};
    const std::string_view view{raw, sizeof(raw)};
    CHECK_EQ(Logger::formatRecord(instant(0), ELogLevel::INFO, view),
             "1970-01-01 00:00:00.000 [INFO] HELLO");
}
