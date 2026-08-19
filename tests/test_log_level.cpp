#include <framework/TestFramework.hpp>

#include <logger/LogLevel.hpp>

#include <cstddef>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>

using Logger::ELogLevel;

namespace {
    constexpr std::size_t kLevelCount = static_cast<std::size_t>(ELogLevel::Count);
    constexpr std::size_t kUnderlyingMax =
        std::numeric_limits<std::underlying_type_t<ELogLevel>>::max();
} // namespace

TEST(log_level, names_match_the_enumeration) {
    CHECK_EQ(Logger::level2string(ELogLevel::Debug), "DEBUG");
    CHECK_EQ(Logger::level2string(ELogLevel::Info), "INFO");
    CHECK_EQ(Logger::level2string(ELogLevel::Warn), "WARN");
    CHECK_EQ(Logger::level2string(ELogLevel::Error), "ERROR");
    CHECK_EQ(Logger::level2string(ELogLevel::Fatal), "FATAL");
}

TEST(log_level, every_level_has_a_name) {
    for (std::size_t index = 0; index < kLevelCount; ++index) {
        const auto level = static_cast<ELogLevel>(index);
        const auto name = Logger::level2string(level);

        CHECK(!name.empty());
        CHECK(name != "UNKNOWN");
    }
}

TEST(log_level, sentinel_and_out_of_range_are_unknown) {
    CHECK_EQ(Logger::level2string(ELogLevel::Count), "UNKNOWN");
    CHECK_EQ(Logger::level2string(static_cast<ELogLevel>(42)), "UNKNOWN");
    CHECK_EQ(Logger::level2string(static_cast<ELogLevel>(-1)), "UNKNOWN");
}

TEST(log_level, name_round_trips_back_to_the_same_level) {
    for (std::size_t index = 0; index < kLevelCount; ++index) {
        const auto level = static_cast<ELogLevel>(index);
        const auto parsed = Logger::string2level(Logger::level2string(level));

        if (!CHECK(parsed.has_value())) continue;
        CHECK_EQ(*parsed, level);
    }
}

TEST(log_level, parsing_ignores_case) {
    const std::optional<ELogLevel> expected{ELogLevel::Info};

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

TEST(log_level, every_enumerator_is_valid) {
    CHECK(Logger::isValidLevel(ELogLevel::Debug));
    CHECK(Logger::isValidLevel(ELogLevel::Info));
    CHECK(Logger::isValidLevel(ELogLevel::Warn));
    CHECK(Logger::isValidLevel(ELogLevel::Error));
    CHECK(Logger::isValidLevel(ELogLevel::Fatal));

    for (std::size_t index = 0; index < kLevelCount; ++index) {
        CHECK(Logger::isValidLevel(static_cast<ELogLevel>(index)));
    }
}

TEST(log_level, sentinel_and_out_of_range_are_invalid) {
    CHECK(!Logger::isValidLevel(ELogLevel::Count));
    CHECK(!Logger::isValidLevel(static_cast<ELogLevel>(kLevelCount + 1)));
    CHECK(!Logger::isValidLevel(static_cast<ELogLevel>(42)));
    CHECK(!Logger::isValidLevel(static_cast<ELogLevel>(-1)));
    CHECK(!Logger::isValidLevel(static_cast<ELogLevel>(kUnderlyingMax)));
}

TEST(log_level, validity_agrees_with_the_name_table) {
    for (std::size_t raw = 0; raw <= kUnderlyingMax; ++raw) {
        const auto level = static_cast<ELogLevel>(raw);
        CHECK_EQ(Logger::isValidLevel(level), Logger::level2string(level) != "UNKNOWN");
    }
}

TEST(log_level, order_matches_severity) {
    CHECK(ELogLevel::Debug < ELogLevel::Info);
    CHECK(ELogLevel::Info < ELogLevel::Warn);
    CHECK(ELogLevel::Warn < ELogLevel::Error);
    CHECK(ELogLevel::Error < ELogLevel::Fatal);
    CHECK(ELogLevel::Fatal < ELogLevel::Count);
}
