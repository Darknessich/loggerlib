#include <framework/TestFramework.hpp>

#include <common/Text.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

using Common::equalsIgnoreCase;
using Common::isDigit;
using Common::parseNumber;
using Common::toLowerCase;

TEST(text, lower_cases_only_ascii_letters) {
    CHECK_EQ(toLowerCase('A'), 'a');
    CHECK_EQ(toLowerCase('Z'), 'z');
    CHECK_EQ(toLowerCase('a'), 'a');
    CHECK_EQ(toLowerCase('7'), '7');
    CHECK_EQ(toLowerCase('\xC0'), '\xC0');
}

TEST(text, compares_ignoring_the_case) {
    CHECK(equalsIgnoreCase("INFO", "info"));
    CHECK(equalsIgnoreCase("Udp", "uDP"));
    CHECK(equalsIgnoreCase("", ""));
    CHECK(!equalsIgnoreCase("info", "warn"));
    CHECK(!equalsIgnoreCase("info", "inf"));
}

TEST(text, knows_a_digit_from_the_rest) {
    for (char c = '0'; c <= '9'; ++c) {
        CHECK(isDigit(c));
    }

    CHECK(!isDigit('a'));
    CHECK(!isDigit(' '));
    CHECK(!isDigit('\xFF'));
}

TEST(text, reads_a_number_whole_or_not_at_all) {
    const auto count = parseNumber<std::size_t>("100");
    REQUIRE(count.has_value());
    CHECK_EQ(*count, std::size_t{100});

    CHECK(!parseNumber<std::size_t>("100x").has_value());
    CHECK(!parseNumber<std::size_t>("x100").has_value());
    CHECK(!parseNumber<std::size_t>(" 100").has_value());
    CHECK(!parseNumber<std::size_t>("").has_value());
}

TEST(text, refuses_what_does_not_fit_the_type) {
    CHECK(!parseNumber<std::uint16_t>("70000").has_value());
    CHECK(!parseNumber<unsigned>("-1").has_value());

    const auto negative = parseNumber<int>("-1");
    REQUIRE(negative.has_value());
    CHECK_EQ(*negative, -1);
}
