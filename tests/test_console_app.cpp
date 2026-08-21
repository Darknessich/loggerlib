#include <framework/TestFramework.hpp>
#include <utils/RecordingLogger.hpp>

#include <core/ConsoleApp.hpp>

#include <logger/LogLevel.hpp>

#include <chrono>
#include <cstddef>
#include <ios>
#include <istream>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

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

TEST(console_app, uses_the_default_level) {
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
    const auto session = run(logger, "/level WARN\nINFO dropped\nERROR kept\n");

    CHECK_EQ(logger.level(), ELogLevel::Warn);
    CHECK(session.out.find("level: WARN") != std::string::npos);

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

TEST(console_app, level_command_spares_queued_messages) {
    constexpr std::size_t kLines = 200;

    RecordingLogger logger{ELogLevel::Debug};

    std::string input;
    for (std::size_t i = 0; i < kLines; ++i) {
        input += "INFO message " + std::to_string(i) + '\n';
    }
    input += "/level ERROR\n";
    input += "INFO dropped\n";

    const auto session = run(logger, input);

    CHECK_EQ(logger.level(), ELogLevel::Error);
    CHECK_EQ(logger.count(), kLines);
    CHECK(session.err.find(std::to_string(kLines) + " message(s), 0 failed") != std::string::npos);
}

TEST(console_app, drops_a_message_below_the_threshold) {
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

TEST(console_app, rejects_an_unknown_level) {
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

TEST(console_app, names_the_failure_reason) {
    RecordingLogger logger{ELogLevel::Debug};
    logger.failWrites(std::make_error_code(std::errc::no_space_on_device));

    const auto session = run(logger, "doomed\n");
    const std::string reason = std::make_error_code(std::errc::no_space_on_device).message();

    REQUIRE_EQ(session.code, EExitCode::WriteFailed);
    CHECK(session.err.find(reason) != std::string::npos);

    CHECK(session.err.find("1 failed: " + reason) != std::string::npos);
}

TEST(console_app, names_the_reason_in_the_loop) {
    class GatedInput : public std::stringbuf {
    public:
        GatedInput(
            const std::string& first,
            std::string rest,
            std::size_t failures,
            const RecordingLogger& logger
        )
            : std::stringbuf{first, std::ios::in}, m_rest{std::move(rest)}, m_failures{failures},
              m_logger{&logger} {}

        [[nodiscard]] bool timedOut() const noexcept { return m_timedOut; }

    protected:
        int_type underflow() override {
            const int_type next = std::stringbuf::underflow();
            if (next != traits_type::eof() || m_rest.empty()) return next;

            m_timedOut = !m_logger->waitForFailures(m_failures, std::chrono::seconds{3});
            str(m_rest);
            m_rest.clear();
            return std::stringbuf::underflow();
        }

    private:
        std::string m_rest;
        std::size_t m_failures;
        const RecordingLogger* m_logger;
        bool m_timedOut = false;
    };

    RecordingLogger logger{ELogLevel::Debug};
    logger.failWrites(std::make_error_code(std::errc::no_space_on_device));

    GatedInput gate{"one\ntwo\n", "three\nfour\n", 2, logger};
    std::istream in{&gate};
    std::ostringstream out;
    std::ostringstream err;

    App::ConsoleApp application{logger, in, out, err, false};
    const EExitCode code = application.run();
    REQUIRE(!gate.timedOut());
    REQUIRE_EQ(code, EExitCode::WriteFailed);

    const std::string reason = std::make_error_code(std::errc::no_space_on_device).message();
    const std::string report = "log write failed: " + reason;
    const std::string text = err.str();

    const auto inLoop = text.find(report);
    const auto summary = text.find("4 failed: " + reason);

    CHECK(inLoop != std::string::npos);
    CHECK(summary != std::string::npos);
    CHECK(inLoop < summary);

    std::size_t reports = 0;
    for (auto at = inLoop; at != std::string::npos; at = text.find(report, at + 1)) {
        ++reports;
    }
    CHECK_EQ(reports, std::size_t{1});
}

TEST(console_app, omits_the_prompt) {
    RecordingLogger logger{ELogLevel::Debug};
    const auto session = run(logger, "one\ntwo\n");

    CHECK_EQ(session.out, "");
}
