#include <framework/TestFramework.hpp>

#include <logger/LogLevel.hpp>

#include <cstddef>
#include <optional>
#include <string_view>

using Logger::ELogLevel;

namespace {
    constexpr std::size_t kLevelCount = static_cast<std::size_t>(ELogLevel::COUNT);
}

TEST(log_level, names_match_the_enumeration) {
    CHECK_EQ(Logger::level2string(ELogLevel::DEBUG), "DEBUG");
    CHECK_EQ(Logger::level2string(ELogLevel::INFO),  "INFO");
    CHECK_EQ(Logger::level2string(ELogLevel::WARN),  "WARN");
    CHECK_EQ(Logger::level2string(ELogLevel::ERROR), "ERROR");
    CHECK_EQ(Logger::level2string(ELogLevel::FATAL), "FATAL");
}

TEST(log_level, every_level_has_a_name) {
    for (std::size_t index = 0; index < kLevelCount; ++index) {
        const auto level = static_cast<ELogLevel>(index);
        const auto name  = Logger::level2string(level);

        CHECK(!name.empty());
        CHECK(name != "UNKNOWN");
    }
}

TEST(log_level, sentinel_and_out_of_range_are_unknown) {
    CHECK_EQ(Logger::level2string(ELogLevel::COUNT), "UNKNOWN");
    CHECK_EQ(Logger::level2string(static_cast<ELogLevel>(42)), "UNKNOWN");
    CHECK_EQ(Logger::level2string(static_cast<ELogLevel>(-1)), "UNKNOWN");
}

TEST(log_level, name_round_trips_back_to_the_same_level) {
    for (std::size_t index = 0; index < kLevelCount; ++index) {
        const auto level  = static_cast<ELogLevel>(index);
        const auto parsed = Logger::string2level(Logger::level2string(level));

        if (!CHECK(parsed.has_value())) continue;
        CHECK_EQ(*parsed, level);
    }
}

TEST(log_level, parsing_ignores_case) {
    const std::optional<ELogLevel> expected{ELogLevel::INFO};

    CHECK_EQ(Logger::string2level("INFO"), expected);
    CHECK_EQ(Logger::string2level("info"), expected);
    CHECK_EQ(Logger::string2level("Info"), expected);
    CHECK_EQ(Logger::string2level("iNfO"), expected);
}

TEST(log_level, unknown_text_is_rejected) {
    CHECK(!Logger::string2level("").has_value());
    CHECK(!Logger::string2level(" ").has_value());

    CHECK(!Logger::string2level("INF").has_value());
    CHECK(!Logger::string2level("INFOS").has_value());

    CHECK(!Logger::string2level("UNKNOWN").has_value());

    CHECK(!Logger::string2level("COUNT").has_value());

    CHECK(!Logger::string2level(" INFO").has_value());
    CHECK(!Logger::string2level("INFO ").has_value());
}

TEST(log_level, non_ascii_input_is_rejected) {
    CHECK(!Logger::string2level("инфо").has_value());
    CHECK(!Logger::string2level("\xD0\x9F").has_value());
}

TEST(log_level, order_matches_severity) {
    CHECK(ELogLevel::DEBUG < ELogLevel::INFO);
    CHECK(ELogLevel::INFO  < ELogLevel::WARN);
    CHECK(ELogLevel::WARN  < ELogLevel::ERROR);
    CHECK(ELogLevel::ERROR < ELogLevel::FATAL);
    CHECK(ELogLevel::FATAL < ELogLevel::COUNT);
}
