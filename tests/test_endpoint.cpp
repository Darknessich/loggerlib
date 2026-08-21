#include <framework/TestFramework.hpp>

#include <common/Endpoint.hpp>

#include <cstdint>

using Common::joinHostPort;
using Common::parsePort;
using Common::splitHostPort;

TEST(endpoint, splits_a_host_and_a_port) {
    const auto parts = splitHostPort("127.0.0.1:5555");

    REQUIRE(parts.has_value());
    CHECK_EQ(parts->host, "127.0.0.1");
    CHECK_EQ(parts->port, "5555");
}

TEST(endpoint, splits_a_name_and_a_port) {
    const auto parts = splitHostPort("logs.example:5555");

    REQUIRE(parts.has_value());
    CHECK_EQ(parts->host, "logs.example");
}

TEST(endpoint, takes_an_ipv6_address_in_brackets) {
    const auto parts = splitHostPort("[::1]:5555");

    REQUIRE(parts.has_value());
    CHECK_EQ(parts->host, "::1");
    CHECK_EQ(parts->port, "5555");
}

TEST(endpoint, refuses_an_ipv6_address_without_brackets) {
    CHECK(!splitHostPort("::1:5555").has_value());
    CHECK(!splitHostPort("[::1]5555").has_value());
}

TEST(endpoint, an_empty_host_is_left_to_the_caller) {
    const auto parts = splitHostPort(":5555");

    REQUIRE(parts.has_value());
    CHECK(parts->host.empty());
    CHECK_EQ(parts->port, "5555");
}

TEST(endpoint, needs_a_port_at_all) {
    CHECK(!splitHostPort("127.0.0.1").has_value());
    CHECK(!splitHostPort("").has_value());
}

TEST(endpoint, reads_a_port) {
    const auto port = parsePort("5555");

    REQUIRE(port.has_value());
    CHECK_EQ(*port, std::uint16_t{5555});
}

TEST(endpoint, refuses_a_port_that_is_not_one) {
    for (const char* text : {"0", "65536", "-1", "55x5", "", " 5555"}) {
        CHECK(!parsePort(text).has_value());
    }
}

TEST(endpoint, joins_what_it_split) {
    for (const char* text : {"127.0.0.1:5555", "[::1]:5555", "[::]:5555", "logs.example:1"}) {
        const auto parts = splitHostPort(text);
        REQUIRE(parts.has_value());

        const auto port = parsePort(parts->port);
        REQUIRE(port.has_value());

        CHECK_EQ(joinHostPort(parts->host, *port), text);
    }
}
