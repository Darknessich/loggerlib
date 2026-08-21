#include <framework/TestFramework.hpp>

#include <core/LineReader.hpp>

#include <cstddef>
#include <string>
#include <vector>

using Collector::LineReader;

TEST(line_reader, splits_a_chunk_holding_several_lines) {
    LineReader reader;
    std::vector<std::string> lines;

    CHECK(reader.feed("first\nsecond\nthird\n", lines));

    REQUIRE_EQ(lines.size(), std::size_t{3});
    CHECK_EQ(lines[0], "first");
    CHECK_EQ(lines[1], "second");
    CHECK_EQ(lines[2], "third");
}

TEST(line_reader, joins_a_line_split_between_chunks) {
    LineReader reader;
    std::vector<std::string> lines;

    CHECK(reader.feed("one hal", lines));
    CHECK(lines.empty());

    CHECK(reader.feed("f and the other\n", lines));

    REQUIRE_EQ(lines.size(), std::size_t{1});
    CHECK_EQ(lines.front(), "one half and the other");
}

TEST(line_reader, keeps_a_tail_without_a_newline) {
    LineReader reader;
    std::vector<std::string> lines;

    CHECK(reader.feed("done\nunfinished", lines));

    REQUIRE_EQ(lines.size(), std::size_t{1});
    CHECK_EQ(lines.front(), "done");

    CHECK(reader.feed("\n", lines));
    REQUIRE_EQ(lines.size(), std::size_t{2});
    CHECK_EQ(lines.back(), "unfinished");
}

TEST(line_reader, an_empty_line_is_a_line) {
    LineReader reader;
    std::vector<std::string> lines;

    CHECK(reader.feed("\n\n", lines));

    REQUIRE_EQ(lines.size(), std::size_t{2});
    CHECK(lines.front().empty());
    CHECK(lines.back().empty());
}

TEST(line_reader, drops_a_line_longer_than_the_limit) {
    LineReader reader{8};
    std::vector<std::string> lines;

    CHECK(!reader.feed("way too long to keep\n", lines));

    CHECK(lines.empty());
}

TEST(line_reader, resynchronises_after_an_overlong_line) {
    LineReader reader{8};
    std::vector<std::string> lines;

    CHECK(!reader.feed("way too long to keep", lines));
    CHECK(reader.feed(" and still going", lines));
    CHECK(lines.empty());

    CHECK(reader.feed("\nshort\n", lines));

    REQUIRE_EQ(lines.size(), std::size_t{1});
    CHECK_EQ(lines.front(), "short");
}
