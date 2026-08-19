#include <framework/TestFramework.hpp>

#include <logger/LogLevel.hpp>
#include <logger/LogRecord.hpp>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <initializer_list>
#include <string>

using Logger::ELogLevel;
using Logger::SLogRecord;

namespace {
    std::chrono::system_clock::time_point instant(std::time_t seconds, int millis = 0) {
        return std::chrono::system_clock::from_time_t(seconds) + std::chrono::milliseconds{millis};
    }

    std::int64_t millisSinceEpoch(std::chrono::system_clock::time_point time) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch())
            .count();
    }
} // namespace

TEST(log_record, formats_known_instants) {
    CHECK_EQ(
        Logger::formatRecord({instant(0, 123), ELogLevel::Info, "hello"}),
        "1970-01-01 00:00:00.123Z [INFO] hello"
    );

    CHECK_EQ(
        Logger::formatRecord({instant(1'000'000'000), ELogLevel::Fatal, "boom"}),
        "2001-09-09 01:46:40.000Z [FATAL] boom"
    );
}

TEST(log_record, formats_empty_message) {
    CHECK_EQ(
        Logger::formatRecord({instant(0), ELogLevel::Warn, ""}), "1970-01-01 00:00:00.000Z [WARN] "
    );
}

TEST(log_record, round_trips_through_format_and_parse) {
    for (const std::string message :
         {"",
          "simple",
          "with  spaces",
          "юникод",
          "a\nb",
          "a\rb",
          "back\\slash",
          "]",
          "[INFO] fake",
          "trailing backslash \\"}) {
        const SLogRecord original{instant(1'000'000'000, 456), ELogLevel::Warn, message};

        const auto parsed = Logger::parseRecord(Logger::formatRecord(original));
        if (!CHECK(parsed.has_value())) continue;

        CHECK_EQ(millisSinceEpoch(parsed->time), millisSinceEpoch(original.time));
        CHECK_EQ(parsed->level, original.level);
        CHECK_EQ(parsed->message, original.message);
    }
}

TEST(log_record, round_trips_every_level) {
    for (std::size_t index = 0; index < static_cast<std::size_t>(ELogLevel::Count); ++index) {
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
    CHECK_EQ(Logger::escapeMessage("\t"), "\\t");

    CHECK(Logger::escapeMessage("a\nb") != Logger::escapeMessage("a\\nb"));

    for (const std::string text :
         {"", "plain", "a\nb", "a\rb", "a\\b", "\\", "\\\\", "\\n", "смешанный \\ текст\n"}) {
        CHECK_EQ(Logger::unescapeMessage(Logger::escapeMessage(text)), text);
    }
}

TEST(log_record, escaping_round_trips_every_byte) {
    for (int value = 0; value <= 0xFF; ++value) {
        const std::string original(1, static_cast<char>(value));
        const std::string escaped = Logger::escapeMessage(original);

        CHECK(escaped.find('\n') == std::string::npos);
        CHECK(escaped.find('\r') == std::string::npos);
        CHECK_EQ(Logger::unescapeMessage(escaped), original);
    }
}

TEST(log_record, escapes_control_characters) {
    CHECK_EQ(Logger::escapeMessage("\x1b[2J"), "\\x1B[2J");
    CHECK_EQ(Logger::escapeMessage(std::string(1, '\0')), "\\x00");
    CHECK_EQ(Logger::escapeMessage("\x7f"), "\\x7F");
    CHECK_EQ(Logger::escapeMessage("\x01\x1f"), "\\x01\\x1F");
}

TEST(log_record, leaves_utf8_untouched) {
    const std::string text = "юникод — ok";
    CHECK_EQ(Logger::escapeMessage(text), text);
}

TEST(log_record, unescaping_accepts_any_hex_escape) {
    CHECK_EQ(Logger::unescapeMessage("\\x21"), "!");
    CHECK_EQ(Logger::unescapeMessage("\\x2a"), "*");
    CHECK_EQ(Logger::escapeMessage("!"), "!");
}

TEST(log_record, unescaping_tolerates_malformed_input) {
    CHECK_EQ(Logger::unescapeMessage("a\\qb"), "a\\qb");
    CHECK_EQ(Logger::unescapeMessage("a\\"), "a\\");
    CHECK_EQ(Logger::unescapeMessage("\\"), "\\");
    CHECK_EQ(Logger::unescapeMessage(""), "");

    CHECK_EQ(Logger::unescapeMessage("\\x"), "\\x");
    CHECK_EQ(Logger::unescapeMessage("\\x1"), "\\x1");
    CHECK_EQ(Logger::unescapeMessage("\\xZZ"), "\\xZZ");
    CHECK_EQ(Logger::unescapeMessage("\\x0Z"), "\\x0Z");
}

TEST(log_record, control_characters_survive_a_record_round_trip) {
    std::string message = "before";
    message += '\x1b';
    message += '\t';
    message += '\0';
    message += "after";

    const std::string line = Logger::formatRecord({instant(0), ELogLevel::Info, message});
    CHECK(line.find('\x1b') == std::string::npos);
    CHECK(line.find('\n') == std::string::npos);

    const auto parsed = Logger::parseRecord(line);
    REQUIRE(parsed.has_value());
    CHECK_EQ(parsed->message, message);

    CHECK(Logger::escapeMessage(message).size() > message.size());
    CHECK_EQ(parsed->message.size(), message.size());
}

TEST(log_record, rejects_short_and_truncated_lines) {
    CHECK(!Logger::parseRecord("").has_value());
    CHECK(!Logger::parseRecord("x").has_value());
    CHECK(!Logger::parseRecord(std::string(23, 'x')).has_value());
    CHECK(!Logger::parseRecord(std::string(24, 'x')).has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.000Z").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.000Z [INFO]").has_value());
}

TEST(log_record, rejects_malformed_structure) {
    CHECK(!Logger::parseRecord("1970-01-01T00:00:00.000Z [INFO] text").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00,000Z [INFO] text").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.000Z INFO text").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.000Z [INFO]text").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.000Z [NOPE] text").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.000Z [] text").has_value());
    CHECK(!Logger::parseRecord("19x0-01-01 00:00:00.000Z [INFO] text").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.00aZ [INFO] text").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:00:00.000Y [INFO] text").has_value());
}

TEST(log_record, rejects_impossible_dates) {
    CHECK(!Logger::parseRecord("1970-13-01 00:00:00.000Z [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1970-00-01 00:00:00.000Z [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1970-01-00 00:00:00.000Z [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1970-01-32 00:00:00.000Z [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1970-04-31 00:00:00.000Z [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 24:00:00.000Z [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 00:60:00.000Z [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1970-01-01 23:59:60.000Z [INFO] x").has_value());
}

TEST(log_record, handles_leap_years) {
    CHECK(Logger::parseRecord("2024-02-29 00:00:00.000Z [INFO] x").has_value());
    CHECK(Logger::parseRecord("2000-02-29 00:00:00.000Z [INFO] x").has_value());
    CHECK(!Logger::parseRecord("2023-02-29 00:00:00.000Z [INFO] x").has_value());
    CHECK(!Logger::parseRecord("1900-02-29 00:00:00.000Z [INFO] x").has_value());
}

TEST(log_record, accepts_boundary_values) {
    CHECK(Logger::parseRecord("1970-01-01 23:59:59.999Z [INFO] x").has_value());
    CHECK(Logger::parseRecord("1970-01-31 00:00:00.000Z [INFO] x").has_value());
    CHECK(Logger::parseRecord("1970-12-31 00:00:00.000Z [INFO] x").has_value());
    CHECK(Logger::parseRecord("1970-01-01 00:00:00.999Z [INFO] x").has_value());
}

TEST(log_record, invalid_level_produces_unparseable_line) {
    const auto line = Logger::formatRecord({instant(0), static_cast<ELogLevel>(42), "x"});
    CHECK_EQ(line, "1970-01-01 00:00:00.000Z [UNKNOWN] x");
    CHECK(!Logger::parseRecord(line).has_value());
}

TEST(log_record, both_format_overloads_agree) {
    const SLogRecord record{instant(1'000'000'000, 456), ELogLevel::Warn, "a\nb"};
    CHECK_EQ(
        Logger::formatRecord(record),
        Logger::formatRecord(record.time, record.level, record.message)
    );
}

TEST(log_record, formats_a_view_without_a_terminator) {
    const char raw[] = {'H', 'E', 'L', 'L', 'O'};
    const std::string_view view{raw, sizeof(raw)};
    CHECK_EQ(
        Logger::formatRecord(instant(0), ELogLevel::Info, view),
        "1970-01-01 00:00:00.000Z [INFO] HELLO"
    );
}

TEST(log_record, output_does_not_depend_on_the_local_time_zone) {
    const auto formatUnder = [](const char* zone) {
        ::setenv("TZ", zone, 1);
        ::tzset();
        return Logger::formatRecord({instant(0, 123), ELogLevel::Info, "hello"});
    };

    const auto moscow = formatUnder("Europe/Moscow");
    const auto chatham = formatUnder("Pacific/Chatham");
    ::unsetenv("TZ");
    ::tzset();

    CHECK_EQ(moscow, "1970-01-01 00:00:00.123Z [INFO] hello");
    CHECK_EQ(chatham, moscow);
}
