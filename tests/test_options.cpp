#include <framework/TestFramework.hpp>

#include <core/Options.hpp>

#include <logger/LogLevel.hpp>

#include <string>
#include <vector>

using App::EOptionsKind;
using Logger::ELogLevel;

namespace {
    App::SOptions parse(std::vector<const char*> argv) {
        return App::parseOptions(static_cast<int>(argv.size()), argv.data());
    }
} // namespace

TEST(options, takes_the_path_and_the_level) {
    const auto parsed = parse({"logger_app", "app.log", "WARN"});

    REQUIRE_EQ(parsed.kind, EOptionsKind::Run);
    CHECK_EQ(parsed.path, "app.log");
    CHECK_EQ(parsed.level, ELogLevel::Warn);
}

TEST(options, level_defaults_to_info) {
    const auto parsed = parse({"logger_app", "app.log"});

    REQUIRE_EQ(parsed.kind, EOptionsKind::Run);
    CHECK_EQ(parsed.path, "app.log");
    CHECK_EQ(parsed.level, ELogLevel::Info);
}

TEST(options, level_is_case_insensitive) {
    const auto parsed = parse({"logger_app", "app.log", "debug"});

    REQUIRE_EQ(parsed.kind, EOptionsKind::Run);
    CHECK_EQ(parsed.level, ELogLevel::Debug);
}

TEST(options, rejects_an_unknown_level) {
    const auto parsed = parse({"logger_app", "app.log", "NOPE"});

    REQUIRE_EQ(parsed.kind, EOptionsKind::Error);
    CHECK(parsed.error.find("NOPE") != std::string::npos);
}

TEST(options, rejects_a_missing_path) {
    const auto parsed = parse({"logger_app"});

    REQUIRE_EQ(parsed.kind, EOptionsKind::Error);
    CHECK(!parsed.error.empty());
}

TEST(options, rejects_an_empty_path) {
    const auto parsed = parse({"logger_app", ""});

    REQUIRE_EQ(parsed.kind, EOptionsKind::Error);
    CHECK(!parsed.error.empty());
}

TEST(options, rejects_an_extra_argument) {
    const auto parsed = parse({"logger_app", "app.log", "INFO", "extra"});

    REQUIRE_EQ(parsed.kind, EOptionsKind::Error);
    CHECK(parsed.error.find("extra") != std::string::npos);
}

TEST(options, rejects_an_unknown_option) {
    const auto parsed = parse({"logger_app", "--verbose", "app.log"});

    REQUIRE_EQ(parsed.kind, EOptionsKind::Error);
    CHECK(parsed.error.find("--verbose") != std::string::npos);
}

TEST(options, reports_a_help_request) {
    for (const auto flag : {"--help", "-h"}) {
        const auto parsed = parse({"logger_app", flag});

        REQUIRE_EQ(parsed.kind, EOptionsKind::Help);
    }
}

TEST(options, help_wins_over_a_missing_path) {
    const auto parsed = parse({"logger_app", "--help"});

    REQUIRE_EQ(parsed.kind, EOptionsKind::Help);
    CHECK(parsed.error.empty());
}
