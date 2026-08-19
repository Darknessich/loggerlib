#include <framework/TestFramework.hpp>

#include <core/InputParser.hpp>

#include <logger/LogLevel.hpp>

using App::EInputKind;
using App::parseUserInput;
using Logger::ELogLevel;

TEST(input_parser, plain_line_takes_the_default_level) {
    const auto input = parseUserInput("hello", ELogLevel::Warn);

    REQUIRE_EQ(input.kind, EInputKind::Message);
    CHECK_EQ(input.level, ELogLevel::Warn);
    CHECK_EQ(input.message, "hello");
}

TEST(input_parser, first_word_sets_the_level) {
    const auto input = parseUserInput("WARN disk almost full", ELogLevel::Info);

    REQUIRE_EQ(input.kind, EInputKind::Message);
    CHECK_EQ(input.level, ELogLevel::Warn);
    CHECK_EQ(input.message, "disk almost full");
}

TEST(input_parser, level_word_is_case_insensitive) {
    const auto input = parseUserInput("warn lower case", ELogLevel::Info);

    REQUIRE_EQ(input.kind, EInputKind::Message);
    CHECK_EQ(input.level, ELogLevel::Warn);
    CHECK_EQ(input.message, "lower case");
}

TEST(input_parser, only_the_first_word_can_be_a_level) {
    const auto input = parseUserInput("the ERROR happened", ELogLevel::Info);

    REQUIRE_EQ(input.kind, EInputKind::Message);
    CHECK_EQ(input.level, ELogLevel::Info);
    CHECK_EQ(input.message, "the ERROR happened");
}

TEST(input_parser, rejects_a_level_without_text) {
    const auto input = parseUserInput("WARN", ELogLevel::Info);

    REQUIRE_EQ(input.kind, EInputKind::Error);
    CHECK(!input.error.empty());
}

TEST(input_parser, surrounding_spaces_are_trimmed) {
    const auto input = parseUserInput("   INFO   keep   inner   ", ELogLevel::Debug);

    REQUIRE_EQ(input.kind, EInputKind::Message);
    CHECK_EQ(input.level, ELogLevel::Info);
    CHECK_EQ(input.message, "keep   inner");
}

TEST(input_parser, blank_lines_are_empty) {
    for (const auto* const line : {"", "   ", "\t"}) {
        const auto input = parseUserInput(line, ELogLevel::Info);
        REQUIRE_EQ(input.kind, EInputKind::Empty);
    }
}

TEST(input_parser, quit_command) {
    const auto input = parseUserInput("/quit", ELogLevel::Info);

    REQUIRE_EQ(input.kind, EInputKind::Quit);
}

TEST(input_parser, help_command) {
    const auto input = parseUserInput("/help", ELogLevel::Info);

    REQUIRE_EQ(input.kind, EInputKind::Help);
}

TEST(input_parser, recognises_a_command_after_spaces) {
    const auto input = parseUserInput("   /quit  ", ELogLevel::Info);

    REQUIRE_EQ(input.kind, EInputKind::Quit);
}

TEST(input_parser, level_command_carries_the_new_level) {
    const auto input = parseUserInput("/level debug", ELogLevel::Info);

    REQUIRE_EQ(input.kind, EInputKind::SetLevel);
    CHECK_EQ(input.level, ELogLevel::Debug);
}

TEST(input_parser, level_command_needs_an_argument) {
    const auto input = parseUserInput("/level", ELogLevel::Info);

    REQUIRE_EQ(input.kind, EInputKind::Error);
    CHECK(!input.error.empty());
}

TEST(input_parser, level_command_rejects_an_unknown_level) {
    const auto input = parseUserInput("/level NOPE", ELogLevel::Info);

    REQUIRE_EQ(input.kind, EInputKind::Error);
    CHECK(input.error.find("NOPE") != std::string::npos);
}

TEST(input_parser, rejects_an_unknown_command) {
    const auto input = parseUserInput("/bogus", ELogLevel::Info);

    REQUIRE_EQ(input.kind, EInputKind::Error);
    CHECK(input.error.find("/bogus") != std::string::npos);
}

TEST(input_parser, double_slash_escapes_a_message) {
    const auto input = parseUserInput("//quit", ELogLevel::Warn);

    REQUIRE_EQ(input.kind, EInputKind::Message);
    CHECK_EQ(input.level, ELogLevel::Warn);
    CHECK_EQ(input.message, "/quit");
}
