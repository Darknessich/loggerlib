#include <framework/TestFramework.hpp>

#include <core/InputParser.hpp>

#include <logger/LogLevel.hpp>

#include <string>
#include <variant>

using App::parseUserInput;
using App::SEmptyLine;
using App::SError;
using App::SHelpCommand;
using App::SLevelCommand;
using App::SMessage;
using App::SQuitCommand;
using Logger::ELogLevel;

TEST(input_parser, plain_line_takes_the_default_level) {
    const auto input = parseUserInput("hello", ELogLevel::Warn);

    REQUIRE(std::holds_alternative<SMessage>(input));
    CHECK_EQ(std::get<SMessage>(input).level, ELogLevel::Warn);
    CHECK_EQ(std::get<SMessage>(input).text, "hello");
}

TEST(input_parser, first_word_sets_the_level) {
    const auto input = parseUserInput("WARN disk almost full", ELogLevel::Info);

    REQUIRE(std::holds_alternative<SMessage>(input));
    CHECK_EQ(std::get<SMessage>(input).level, ELogLevel::Warn);
    CHECK_EQ(std::get<SMessage>(input).text, "disk almost full");
}

TEST(input_parser, level_word_is_case_insensitive) {
    const auto input = parseUserInput("warn lower case", ELogLevel::Info);

    REQUIRE(std::holds_alternative<SMessage>(input));
    CHECK_EQ(std::get<SMessage>(input).level, ELogLevel::Warn);
    CHECK_EQ(std::get<SMessage>(input).text, "lower case");
}

TEST(input_parser, only_the_first_word_can_be_a_level) {
    const auto input = parseUserInput("the ERROR happened", ELogLevel::Info);

    REQUIRE(std::holds_alternative<SMessage>(input));
    CHECK_EQ(std::get<SMessage>(input).level, ELogLevel::Info);
    CHECK_EQ(std::get<SMessage>(input).text, "the ERROR happened");
}

TEST(input_parser, rejects_a_level_without_text) {
    const auto input = parseUserInput("WARN", ELogLevel::Info);

    REQUIRE(std::holds_alternative<SError>(input));
    CHECK(!std::get<SError>(input).text.empty());
}

TEST(input_parser, surrounding_spaces_are_trimmed) {
    const auto input = parseUserInput("   INFO   keep   inner   ", ELogLevel::Debug);

    REQUIRE(std::holds_alternative<SMessage>(input));
    CHECK_EQ(std::get<SMessage>(input).level, ELogLevel::Info);
    CHECK_EQ(std::get<SMessage>(input).text, "keep   inner");
}

TEST(input_parser, blank_lines_are_empty) {
    for (const auto* const line : {"", "   ", "\t"}) {
        const auto input = parseUserInput(line, ELogLevel::Info);
        CHECK(std::holds_alternative<SEmptyLine>(input));
    }
}

TEST(input_parser, quit_command) {
    const auto input = parseUserInput("/quit", ELogLevel::Info);

    CHECK(std::holds_alternative<SQuitCommand>(input));
}

TEST(input_parser, help_command) {
    const auto input = parseUserInput("/help", ELogLevel::Info);

    CHECK(std::holds_alternative<SHelpCommand>(input));
}

TEST(input_parser, recognises_a_command_after_spaces) {
    const auto input = parseUserInput("   /quit  ", ELogLevel::Info);

    CHECK(std::holds_alternative<SQuitCommand>(input));
}

TEST(input_parser, level_command_carries_the_new_level) {
    const auto input = parseUserInput("/level debug", ELogLevel::Info);

    REQUIRE(std::holds_alternative<SLevelCommand>(input));
    CHECK_EQ(std::get<SLevelCommand>(input).level, ELogLevel::Debug);
}

TEST(input_parser, level_command_needs_an_argument) {
    const auto input = parseUserInput("/level", ELogLevel::Info);

    REQUIRE(std::holds_alternative<SError>(input));
    CHECK(!std::get<SError>(input).text.empty());
}

TEST(input_parser, level_command_rejects_an_unknown_level) {
    const auto input = parseUserInput("/level NOPE", ELogLevel::Info);

    REQUIRE(std::holds_alternative<SError>(input));
    CHECK(std::get<SError>(input).text.find("NOPE") != std::string::npos);
}

TEST(input_parser, rejects_an_unknown_command) {
    const auto input = parseUserInput("/bogus", ELogLevel::Info);

    REQUIRE(std::holds_alternative<SError>(input));
    CHECK(std::get<SError>(input).text.find("/bogus") != std::string::npos);
}

TEST(input_parser, double_slash_escapes_a_message) {
    const auto input = parseUserInput("//quit", ELogLevel::Warn);

    REQUIRE(std::holds_alternative<SMessage>(input));
    CHECK_EQ(std::get<SMessage>(input).level, ELogLevel::Warn);
    CHECK_EQ(std::get<SMessage>(input).text, "/quit");
}
