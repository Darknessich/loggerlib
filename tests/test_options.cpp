#include <framework/TestFramework.hpp>

#include <core/Options.hpp>

#include <logger/LogLevel.hpp>

#include <string>
#include <variant>
#include <vector>

using App::SRun;
using App::SShowHelp;
using App::SUsageError;
using Logger::ELogLevel;

namespace {
    App::TOptions parse(std::vector<const char*> argv) {
        return App::parseOptions(static_cast<int>(argv.size()), argv.data());
    }
} // namespace

TEST(options, takes_the_path_and_the_level) {
    const auto parsed = parse({"logger_app", "app.log", "WARN"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    CHECK_EQ(std::get<SRun>(parsed).path, "app.log");
    CHECK_EQ(std::get<SRun>(parsed).level, ELogLevel::Warn);
}

TEST(options, level_defaults_to_info) {
    const auto parsed = parse({"logger_app", "app.log"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    CHECK_EQ(std::get<SRun>(parsed).path, "app.log");
    CHECK_EQ(std::get<SRun>(parsed).level, ELogLevel::Info);
}

TEST(options, level_is_case_insensitive) {
    const auto parsed = parse({"logger_app", "app.log", "debug"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    CHECK_EQ(std::get<SRun>(parsed).level, ELogLevel::Debug);
}

TEST(options, rejects_an_unknown_level) {
    const auto parsed = parse({"logger_app", "app.log", "NOPE"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(std::get<SUsageError>(parsed).text.find("NOPE") != std::string::npos);
}

TEST(options, rejects_a_missing_path) {
    const auto parsed = parse({"logger_app"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(!std::get<SUsageError>(parsed).text.empty());
}

TEST(options, rejects_an_empty_path) {
    const auto parsed = parse({"logger_app", ""});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(!std::get<SUsageError>(parsed).text.empty());
}

TEST(options, rejects_an_extra_argument) {
    const auto parsed = parse({"logger_app", "app.log", "INFO", "extra"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(std::get<SUsageError>(parsed).text.find("extra") != std::string::npos);
}

TEST(options, rejects_an_unknown_option) {
    const auto parsed = parse({"logger_app", "--verbose", "app.log"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(std::get<SUsageError>(parsed).text.find("--verbose") != std::string::npos);
}

TEST(options, reports_a_help_request) {
    for (const auto* const flag : {"--help", "-h"}) {
        const auto parsed = parse({"logger_app", flag});

        CHECK(std::holds_alternative<SShowHelp>(parsed));
    }
}

TEST(options, help_wins_over_a_missing_path) {
    const auto parsed = parse({"logger_app", "--help"});

    CHECK(std::holds_alternative<SShowHelp>(parsed));
}

TEST(options, double_dash_ends_the_options) {
    const auto file = parse({"logger_app", "--", "-x.log", "WARN"});

    REQUIRE(std::holds_alternative<SRun>(file));
    CHECK_EQ(std::get<SRun>(file).path, "-x.log");
    CHECK_EQ(std::get<SRun>(file).level, ELogLevel::Warn);

    const auto help = parse({"logger_app", "--", "--help"});

    REQUIRE(std::holds_alternative<SRun>(help));
    CHECK_EQ(std::get<SRun>(help).path, "--help");
}

TEST(options, double_dash_alone_is_not_a_path) {
    const auto parsed = parse({"logger_app", "--"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(std::get<SUsageError>(parsed).text.find("required") != std::string::npos);
}

TEST(options, help_before_a_double_dash_wins) {
    const auto parsed = parse({"logger_app", "--help", "--", "-x.log"});

    CHECK(std::holds_alternative<SShowHelp>(parsed));
}
