#include <framework/TestFramework.hpp>

#include <core/Options.hpp>

#include <logger/LogLevel.hpp>

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

using App::SRun;
using App::SShowHelp;
using App::SUsageError;
using Logger::ELogLevel;
using Logger::SFileTarget;
using Logger::SSocketTarget;

namespace {
    const SFileTarget& fileTarget(const App::TOptions& parsed, std::size_t index = 0) {
        return std::get<SFileTarget>(std::get<SRun>(parsed).targets.at(index));
    }

    const SSocketTarget& socketTarget(const App::TOptions& parsed, std::size_t index = 0) {
        return std::get<SSocketTarget>(std::get<SRun>(parsed).targets.at(index));
    }

    App::TOptions parse(std::vector<const char*> argv) {
        return App::parseOptions(static_cast<int>(argv.size()), argv.data());
    }
} // namespace

TEST(options, takes_the_path_and_the_level) {
    const auto parsed = parse({"logger_app", "app.log", "WARN"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    CHECK_EQ(fileTarget(parsed).path, "app.log");
    CHECK_EQ(std::get<SRun>(parsed).level, ELogLevel::Warn);
}

TEST(options, level_defaults_to_info) {
    const auto parsed = parse({"logger_app", "app.log"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    CHECK_EQ(fileTarget(parsed).path, "app.log");
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

TEST(options, double_dash_ends_the_options) {
    const auto file = parse({"logger_app", "--", "-x.log", "WARN"});

    REQUIRE(std::holds_alternative<SRun>(file));
    CHECK_EQ(fileTarget(file).path, "-x.log");
    CHECK_EQ(std::get<SRun>(file).level, ELogLevel::Warn);

    const auto help = parse({"logger_app", "--", "--help"});

    REQUIRE(std::holds_alternative<SRun>(help));
    CHECK_EQ(fileTarget(help).path, "--help");
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

TEST(options, takes_a_socket_target) {
    const auto parsed = parse({"logger_app", "--socket", "127.0.0.1:5555"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    REQUIRE_EQ(std::get<SRun>(parsed).targets.size(), std::size_t{1});
    CHECK_EQ(socketTarget(parsed).host, "127.0.0.1");
    CHECK_EQ(socketTarget(parsed).port, 5555);
    CHECK_EQ(socketTarget(parsed).protocol, Logger::ESocketProtocol::Tcp);
}

TEST(options, takes_the_protocol) {
    const auto parsed = parse({"logger_app", "--socket", "host:1", "--proto", "UDP"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    CHECK_EQ(socketTarget(parsed).protocol, Logger::ESocketProtocol::Udp);
}

TEST(options, takes_a_file_and_a_socket_together) {
    const auto parsed = parse({"logger_app", "app.log", "--socket", "host:1", "WARN"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    REQUIRE_EQ(std::get<SRun>(parsed).targets.size(), std::size_t{2});
    CHECK_EQ(fileTarget(parsed, 0).path, "app.log");
    CHECK_EQ(socketTarget(parsed, 1).host, "host");
    CHECK_EQ(std::get<SRun>(parsed).level, ELogLevel::Warn);
}

TEST(options, takes_a_bracketed_ipv6_address) {
    const auto parsed = parse({"logger_app", "--socket", "[::1]:5555"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    CHECK_EQ(socketTarget(parsed).host, "::1");
    CHECK_EQ(socketTarget(parsed).port, 5555);
}

TEST(options, rejects_an_ipv6_address_without_brackets) {
    const auto parsed = parse({"logger_app", "--socket", "::1:5555"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(std::get<SUsageError>(parsed).text.find("brackets") != std::string::npos);
}

TEST(options, rejects_a_bad_endpoint) {
    for (const auto* const value : {"host", "host:", "host:0", "host:65536", "host:abc", ":5555"}) {
        const auto parsed = parse({"logger_app", "--socket", value});
        CHECK(std::holds_alternative<SUsageError>(parsed));
    }
}

TEST(options, rejects_an_unknown_protocol) {
    const auto parsed = parse({"logger_app", "--socket", "host:1", "--proto", "bogus"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(std::get<SUsageError>(parsed).text.find("bogus") != std::string::npos);
}

TEST(options, rejects_a_protocol_without_a_socket) {
    const auto parsed = parse({"logger_app", "app.log", "--proto", "udp"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(std::get<SUsageError>(parsed).text.find("--socket") != std::string::npos);
}

TEST(options, rejects_a_missing_option_value) {
    for (const auto* const flag : {"--file", "--socket", "--proto", "--level"}) {
        const auto parsed = parse({"logger_app", flag});
        CHECK(std::holds_alternative<SUsageError>(parsed));
    }
}

TEST(options, rejects_a_repeated_protocol) {
    const auto parsed =
        parse({"logger_app", "--socket", "a:1", "--proto", "tcp", "--proto", "udp"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(std::get<SUsageError>(parsed).text.find("--proto") != std::string::npos);
}

TEST(options, takes_several_files) {
    const auto parsed = parse({"logger_app", "a.log", "--file", "b.log", "--file", "c.log"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    REQUIRE_EQ(std::get<SRun>(parsed).targets.size(), std::size_t{3});
    CHECK_EQ(fileTarget(parsed, 0).path, "a.log");
    CHECK_EQ(fileTarget(parsed, 1).path, "b.log");
    CHECK_EQ(fileTarget(parsed, 2).path, "c.log");
}

TEST(options, takes_several_sockets) {
    const auto parsed = parse({"logger_app", "--socket", "a:1", "--socket", "b:2"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    REQUIRE_EQ(std::get<SRun>(parsed).targets.size(), std::size_t{2});
    CHECK_EQ(socketTarget(parsed, 0).host, "a");
    CHECK_EQ(socketTarget(parsed, 1).port, 2);
}

TEST(options, each_socket_keeps_its_own_protocol) {
    const auto parsed =
        parse({"logger_app", "--socket", "a:1", "--proto", "udp", "--socket", "b:2"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    CHECK_EQ(socketTarget(parsed, 0).protocol, Logger::ESocketProtocol::Udp);
    CHECK_EQ(socketTarget(parsed, 1).protocol, Logger::ESocketProtocol::Tcp);
}

TEST(options, rejects_a_protocol_before_its_socket) {
    const auto parsed = parse({"logger_app", "--proto", "udp", "--socket", "a:1"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(std::get<SUsageError>(parsed).text.find("--socket") != std::string::npos);
}

TEST(options, rejects_an_empty_file_option) {
    const auto parsed = parse({"logger_app", "--file", ""});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(!std::get<SUsageError>(parsed).text.empty());
}

TEST(options, takes_the_level_as_an_option) {
    const auto parsed = parse({"logger_app", "--socket", "a:1", "--level", "warn"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    REQUIRE_EQ(std::get<SRun>(parsed).targets.size(), std::size_t{1});
    CHECK_EQ(std::get<SRun>(parsed).level, ELogLevel::Warn);
}

TEST(options, rejects_a_repeated_level) {
    const auto parsed = parse({"logger_app", "app.log", "INFO", "--level", "WARN"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(std::get<SUsageError>(parsed).text.find("WARN") != std::string::npos);
}
