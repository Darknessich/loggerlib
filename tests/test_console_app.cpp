#include <framework/TestFramework.hpp>
#include <utils/RecordingLogger.hpp>

#include <core/ConsoleApp.hpp>

#include <logger/LogLevel.hpp>

#include <cstddef>
#include <sstream>
#include <string>

using App::EExitCode;
using Logger::ELogLevel;
using utils::RecordingLogger;

namespace {
    struct SSession {
        EExitCode code{EExitCode::Success};
        std::string out;
        std::string err;
    };

    SSession run(RecordingLogger& logger, const std::string& input) {
        std::istringstream in{input};
        std::ostringstream out;
        std::ostringstream err;

        App::ConsoleApp application{logger, in, out, err, false};
        const EExitCode code = application.run();

        return {code, out.str(), err.str()};
    }
} // namespace

TEST(console_app, logs_plain_lines_at_the_default_level) {
    RecordingLogger logger{ELogLevel::Info};
    const auto session = run(logger, "first\nsecond\n");

    CHECK_EQ(session.code, EExitCode::Success);

    const auto records = logger.records();
    REQUIRE_EQ(records.size(), std::size_t{2});
    CHECK_EQ(records[0].message, "first");
    CHECK_EQ(records[0].level, ELogLevel::Info);
    CHECK_EQ(records[1].message, "second");
}

TEST(console_app, takes_the_level_from_the_first_word) {
    RecordingLogger logger{ELogLevel::Debug};
    run(logger, "WARN disk almost full\n");

    const auto records = logger.records();
    REQUIRE_EQ(records.size(), std::size_t{1});
    CHECK_EQ(records[0].level, ELogLevel::Warn);
    CHECK_EQ(records[0].message, "disk almost full");
}

TEST(console_app, level_command_raises_the_threshold) {
    RecordingLogger logger{ELogLevel::Info};
    run(logger, "/level WARN\nINFO dropped\nERROR kept\n");

    CHECK_EQ(logger.level(), ELogLevel::Warn);

    const auto records = logger.records();
    REQUIRE_EQ(records.size(), std::size_t{1});
    CHECK_EQ(records[0].message, "kept");
}

TEST(console_app, level_command_lowers_the_threshold) {
    RecordingLogger logger{ELogLevel::Warn};
    run(logger, "DEBUG dropped\n/level DEBUG\nDEBUG kept\n");

    CHECK_EQ(logger.level(), ELogLevel::Debug);

    const auto records = logger.records();
    REQUIRE_EQ(records.size(), std::size_t{1});
    CHECK_EQ(records[0].message, "kept");
}

TEST(console_app, a_message_below_the_threshold_never_reaches_the_logger) {
    RecordingLogger logger{ELogLevel::Warn};
    run(logger, "DEBUG hidden\nINFO hidden too\n");

    CHECK_EQ(logger.count(), std::size_t{0});
}

TEST(console_app, quit_stops_reading) {
    RecordingLogger logger{ELogLevel::Debug};
    run(logger, "before\n/quit\nafter\n");

    const auto records = logger.records();
    REQUIRE_EQ(records.size(), std::size_t{1});
    CHECK_EQ(records[0].message, "before");
}

TEST(console_app, nothing_is_lost_when_the_input_ends) {
    constexpr std::size_t kLines = 200;

    RecordingLogger logger{ELogLevel::Debug};

    std::string input;
    for (std::size_t i = 0; i < kLines; ++i) {
        input += "message " + std::to_string(i) + '\n';
    }
    run(logger, input);

    REQUIRE_EQ(logger.count(), kLines);
}

TEST(console_app, blank_lines_are_ignored) {
    RecordingLogger logger{ELogLevel::Debug};
    const auto session = run(logger, "\n   \nreal\n");

    CHECK_EQ(logger.count(), std::size_t{1});
    CHECK(session.err.find("unknown") == std::string::npos);
}

TEST(console_app, reports_an_unknown_command) {
    RecordingLogger logger{ELogLevel::Debug};
    const auto session = run(logger, "/bogus\n");

    CHECK_EQ(logger.count(), std::size_t{0});
    CHECK(session.err.find("/bogus") != std::string::npos);
}

TEST(console_app, an_unknown_level_leaves_the_threshold_alone) {
    RecordingLogger logger{ELogLevel::Info};
    const auto session = run(logger, "/level NOPE\n");

    CHECK_EQ(logger.level(), ELogLevel::Info);
    CHECK(session.err.find("NOPE") != std::string::npos);
}

TEST(console_app, double_slash_logs_a_literal_slash) {
    RecordingLogger logger{ELogLevel::Debug};
    run(logger, "//quit\n");

    const auto records = logger.records();
    REQUIRE_EQ(records.size(), std::size_t{1});
    CHECK_EQ(records[0].message, "/quit");
}

TEST(console_app, help_goes_to_the_output_stream) {
    RecordingLogger logger{ELogLevel::Debug};
    const auto session = run(logger, "/help\n");

    CHECK(session.out.find("/quit") != std::string::npos);
    CHECK_EQ(logger.count(), std::size_t{0});
}

TEST(console_app, reports_write_failures) {
    RecordingLogger logger{ELogLevel::Debug};
    logger.failWrites();

    const auto session = run(logger, "one\ntwo\n");

    REQUIRE_EQ(session.code, EExitCode::WriteFailed);
    CHECK(session.err.find("2 failed") != std::string::npos);
}

TEST(console_app, prints_no_prompt_when_it_is_switched_off) {
    RecordingLogger logger{ELogLevel::Debug};
    const auto session = run(logger, "one\ntwo\n");

    CHECK_EQ(session.out, "");
}
