#include <framework/TestFramework.hpp>

#include <core/Options.hpp>

#include <logger/SocketProtocol.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <variant>
#include <vector>

using Collector::SRun;
using Collector::SShowHelp;
using Collector::SUsageError;
using Logger::ESocketProtocol;

namespace {
    Collector::TOptions parse(std::vector<const char*> argv) {
        return Collector::parseOptions(static_cast<int>(argv.size()), argv.data());
    }

    const SRun& run(const Collector::TOptions& parsed) {
        return std::get<SRun>(parsed);
    }

    const std::string& error(const Collector::TOptions& parsed) {
        return std::get<SUsageError>(parsed).text;
    }
} // namespace

TEST(collector_options, takes_the_address_positionally) {
    const auto parsed = parse({"logger_collector", "127.0.0.1:5555"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    CHECK_EQ(run(parsed).host, "127.0.0.1");
    CHECK_EQ(run(parsed).port, std::uint16_t{5555});
    CHECK_EQ(run(parsed).protocol, ESocketProtocol::Tcp);
    CHECK_EQ(run(parsed).every, std::size_t{100});
    CHECK_EQ(run(parsed).timeout, std::chrono::seconds{10});
}

TEST(collector_options, takes_the_count_and_the_timeout_positionally) {
    const auto parsed = parse({"logger_collector", "127.0.0.1:5555", "5", "3"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    CHECK_EQ(run(parsed).every, std::size_t{5});
    CHECK_EQ(run(parsed).timeout, std::chrono::seconds{3});
}

TEST(collector_options, takes_the_same_values_as_options) {
    const auto parsed =
        parse({"logger_collector", "--listen", "127.0.0.1:5555", "--count", "5", "--timeout", "3"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    CHECK_EQ(run(parsed).host, "127.0.0.1");
    CHECK_EQ(run(parsed).every, std::size_t{5});
    CHECK_EQ(run(parsed).timeout, std::chrono::seconds{3});
}

TEST(collector_options, an_empty_host_means_every_interface) {
    const auto parsed = parse({"logger_collector", ":5555"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    CHECK(run(parsed).host.empty());
    CHECK_EQ(run(parsed).port, std::uint16_t{5555});
}

TEST(collector_options, takes_a_bracketed_ipv6_address) {
    const auto parsed = parse({"logger_collector", "[::1]:5555"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    CHECK_EQ(run(parsed).host, "::1");
    CHECK_EQ(run(parsed).port, std::uint16_t{5555});
}

TEST(collector_options, rejects_an_unbracketed_ipv6_address) {
    const auto parsed = parse({"logger_collector", "::1:5555"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(error(parsed).find("brackets") != std::string::npos);
}

TEST(collector_options, rejects_a_bad_port) {
    for (const char* address : {"127.0.0.1:0", "127.0.0.1:65536", "127.0.0.1:abc", "127.0.0.1:"}) {
        const auto parsed = parse({"logger_collector", address});

        REQUIRE(std::holds_alternative<SUsageError>(parsed));
        CHECK(error(parsed).find("port") != std::string::npos);
    }
}

TEST(collector_options, requires_an_address) {
    const auto parsed = parse({"logger_collector"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(error(parsed).find("address") != std::string::npos);
}

TEST(collector_options, takes_the_protocol) {
    const auto parsed = parse({"logger_collector", "127.0.0.1:5555", "--proto", "udp"});

    REQUIRE(std::holds_alternative<SRun>(parsed));
    CHECK_EQ(run(parsed).protocol, ESocketProtocol::Udp);
}

TEST(collector_options, rejects_an_unknown_protocol) {
    const auto parsed = parse({"logger_collector", "127.0.0.1:5555", "--proto", "carrier-pigeon"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(error(parsed).find("carrier-pigeon") != std::string::npos);
}

TEST(collector_options, rejects_a_count_that_is_not_a_positive_number) {
    for (const char* count : {"0", "-1", "many", ""}) {
        const auto parsed = parse({"logger_collector", "127.0.0.1:5555", "--count", count});

        REQUIRE(std::holds_alternative<SUsageError>(parsed));
        CHECK(error(parsed).find("count") != std::string::npos);
    }
}

TEST(collector_options, rejects_a_timeout_that_is_not_a_positive_number) {
    for (const char* timeout : {"0", "-1", "soon", ""}) {
        const auto parsed = parse({"logger_collector", "127.0.0.1:5555", "--timeout", timeout});

        REQUIRE(std::holds_alternative<SUsageError>(parsed));
        CHECK(error(parsed).find("timeout") != std::string::npos);
    }
}

TEST(collector_options, rejects_a_repeated_value) {
    const auto twice = parse({"logger_collector", "127.0.0.1:5555", "--listen", "127.0.0.1:6666"});
    REQUIRE(std::holds_alternative<SUsageError>(twice));
    CHECK(error(twice).find("twice") != std::string::npos);

    const auto counted = parse({"logger_collector", "1.2.3.4:5555", "5", "--count", "7"});
    REQUIRE(std::holds_alternative<SUsageError>(counted));
    CHECK(error(counted).find("twice") != std::string::npos);
}

TEST(collector_options, rejects_a_fourth_argument) {
    const auto parsed = parse({"logger_collector", "127.0.0.1:5555", "5", "3", "extra"});

    REQUIRE(std::holds_alternative<SUsageError>(parsed));
    CHECK(error(parsed).find("extra") != std::string::npos);
}

TEST(collector_options, reports_an_unknown_option_and_a_missing_value) {
    const auto unknown = parse({"logger_collector", "127.0.0.1:5555", "--verbose"});
    REQUIRE(std::holds_alternative<SUsageError>(unknown));
    CHECK(error(unknown).find("--verbose") != std::string::npos);

    const auto missing = parse({"logger_collector", "--listen"});
    REQUIRE(std::holds_alternative<SUsageError>(missing));
    CHECK(error(missing).find("--listen") != std::string::npos);
}

TEST(collector_options, reports_a_help_request) {
    CHECK(std::holds_alternative<SShowHelp>(parse({"logger_collector", "--help"})));
    CHECK(std::holds_alternative<SShowHelp>(parse({"logger_collector", "-h"})));
    CHECK(std::holds_alternative<SShowHelp>(parse({"logger_collector", "127.0.0.1:5555", "-h"})));
}
