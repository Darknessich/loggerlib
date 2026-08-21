#include <framework/TestFramework.hpp>

#include <core/Statistics.hpp>

#include <logger/LogLevel.hpp>

#include <chrono>
#include <cstddef>
#include <string>

using Collector::formatReport;
using Collector::Statistics;
using Collector::TClock;
using Logger::ELogLevel;

namespace {
    constexpr TClock::time_point kStart{};

    TClock::time_point atMinute(int minute) {
        return kStart + std::chrono::minutes{minute};
    }

    std::size_t countOf(const Collector::SSnapshot& snapshot, ELogLevel level) {
        return snapshot.byLevel.at(static_cast<std::size_t>(level));
    }
} // namespace

TEST(statistics, counts_every_record_by_level) {
    Statistics stats;
    stats.add(ELogLevel::Info, 5, kStart);
    stats.add(ELogLevel::Info, 5, kStart);
    stats.add(ELogLevel::Error, 5, kStart);

    const auto snapshot = stats.snapshot(kStart);
    CHECK_EQ(snapshot.total, std::size_t{3});
    CHECK_EQ(countOf(snapshot, ELogLevel::Info), std::size_t{2});
    CHECK_EQ(countOf(snapshot, ELogLevel::Error), std::size_t{1});
    CHECK_EQ(countOf(snapshot, ELogLevel::Debug), std::size_t{0});
}

TEST(statistics, counts_malformed_lines_apart_from_records) {
    Statistics stats;
    stats.add(ELogLevel::Info, 5, kStart);
    stats.addMalformed();
    stats.addMalformed();

    const auto snapshot = stats.snapshot(kStart);
    CHECK_EQ(snapshot.total, std::size_t{1});
    CHECK_EQ(snapshot.malformed, std::size_t{2});
    CHECK_EQ(snapshot.lastHour, std::size_t{1});
}

TEST(statistics, one_record_is_its_own_minimum_and_maximum) {
    Statistics stats;
    stats.add(ELogLevel::Info, 42, kStart);

    const auto snapshot = stats.snapshot(kStart);
    CHECK_EQ(snapshot.minLength, std::size_t{42});
    CHECK_EQ(snapshot.maxLength, std::size_t{42});
    CHECK_EQ(snapshot.averageLength, 42.0);
}

TEST(statistics, follows_the_lengths) {
    Statistics stats;
    stats.add(ELogLevel::Info, 10, kStart);
    stats.add(ELogLevel::Info, 4, kStart);
    stats.add(ELogLevel::Info, 7, kStart);

    const auto snapshot = stats.snapshot(kStart);
    CHECK_EQ(snapshot.minLength, std::size_t{4});
    CHECK_EQ(snapshot.maxLength, std::size_t{10});
    CHECK_EQ(snapshot.averageLength, 7.0);
}

TEST(statistics, an_empty_snapshot_has_no_lengths) {
    const Statistics stats;

    const auto snapshot = stats.snapshot(kStart);
    CHECK_EQ(snapshot.total, std::size_t{0});
    CHECK_EQ(snapshot.lastHour, std::size_t{0});
    CHECK_EQ(snapshot.averageLength, 0.0);
}

TEST(statistics, the_hour_keeps_a_record_of_fifty_nine_minutes_ago) {
    Statistics stats;
    stats.add(ELogLevel::Info, 5, kStart);

    CHECK_EQ(stats.snapshot(atMinute(59)).lastHour, std::size_t{1});
}

TEST(statistics, the_hour_drops_a_record_of_sixty_minutes_ago) {
    Statistics stats;
    stats.add(ELogLevel::Info, 5, kStart);

    const auto snapshot = stats.snapshot(atMinute(60));
    CHECK_EQ(snapshot.lastHour, std::size_t{0});
    CHECK_EQ(snapshot.total, std::size_t{1});
}

TEST(statistics, a_reused_minute_does_not_carry_the_old_count) {
    Statistics stats;
    stats.add(ELogLevel::Info, 5, kStart);
    stats.add(ELogLevel::Info, 5, atMinute(60));

    CHECK_EQ(stats.snapshot(atMinute(60)).lastHour, std::size_t{1});
}

TEST(statistics, the_hour_spans_the_whole_window) {
    Statistics stats;
    for (int minute = 0; minute < 120; ++minute) {
        stats.add(ELogLevel::Info, 5, atMinute(minute));
    }

    const auto snapshot = stats.snapshot(atMinute(119));
    CHECK_EQ(snapshot.total, std::size_t{120});
    CHECK_EQ(snapshot.lastHour, std::size_t{60});
}

TEST(statistics, the_report_names_every_number) {
    Statistics stats;
    stats.add(ELogLevel::Warn, 10, kStart);
    stats.add(ELogLevel::Warn, 5, kStart);
    stats.addMalformed();

    const std::string report = formatReport(stats.snapshot(kStart), "after 2 messages");

    CHECK(report.find("after 2 messages") != std::string::npos);
    CHECK(report.find("total 2") != std::string::npos);
    CHECK(report.find("last hour 2") != std::string::npos);
    CHECK(report.find("malformed 1") != std::string::npos);
    CHECK(report.find("WARN 2") != std::string::npos);
    CHECK(report.find("DEBUG 0") != std::string::npos);
    CHECK(report.find("min 5") != std::string::npos);
    CHECK(report.find("max 10") != std::string::npos);
    CHECK(report.find("average 7.5") != std::string::npos);
}

TEST(statistics, the_report_of_an_empty_snapshot_shows_no_lengths) {
    const Statistics stats;

    const std::string report = formatReport(stats.snapshot(kStart), "final");

    CHECK(report.find("total 0") != std::string::npos);
    CHECK(report.find("average -") != std::string::npos);
}
