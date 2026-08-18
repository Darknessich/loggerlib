#include <framework/TestFramework.hpp>

#include <LoggerBase.hpp>
#include <logger/LogLevel.hpp>
#include <logger/LogRecord.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using Logger::ELogLevel;

namespace {
    class RecordingLogger : public Logger::LoggerBase {
    public:
        explicit RecordingLogger(ELogLevel level) : LoggerBase(level) {}

        std::vector<std::string> lines;
        bool writeResult = true;

        static std::chrono::system_clock::time_point fixedTime() {
            return std::chrono::system_clock::from_time_t(0);
        }

    protected:
        bool writeLine(std::string_view line) override {
            lines.emplace_back(line);
            return writeResult;
        }

        [[nodiscard]] std::chrono::system_clock::time_point now() const noexcept override {
            return fixedTime();
        }
    };
} // namespace

TEST(logger_base, keeps_messages_at_or_above_the_threshold) {
    RecordingLogger logger{ELogLevel::Warn};

    CHECK(logger.log(ELogLevel::Warn, "warn"));
    CHECK(logger.log(ELogLevel::Error, "error"));
    CHECK(logger.log(ELogLevel::Fatal, "fatal"));

    CHECK_EQ(logger.lines.size(), std::size_t{3});
}

TEST(logger_base, drops_messages_below_the_threshold) {
    RecordingLogger logger{ELogLevel::Warn};

    CHECK(logger.log(ELogLevel::Debug, "debug"));
    CHECK(logger.log(ELogLevel::Info, "info"));

    CHECK_EQ(logger.lines.size(), std::size_t{0});
}

TEST(logger_base, line_matches_format_record) {
    RecordingLogger logger{ELogLevel::Debug};
    CHECK(logger.log(ELogLevel::Info, "hello"));

    REQUIRE_EQ(logger.lines.size(), std::size_t{1});
    CHECK_EQ(
        logger.lines.front(),
        Logger::formatRecord(RecordingLogger::fixedTime(), ELogLevel::Info, "hello")
    );
}

TEST(logger_base, set_level_takes_effect_immediately) {
    RecordingLogger logger{ELogLevel::Warn};

    CHECK(logger.log(ELogLevel::Info, "before"));
    CHECK_EQ(logger.lines.size(), std::size_t{0});

    logger.setLevel(ELogLevel::Debug);
    CHECK_EQ(logger.level(), ELogLevel::Debug);

    CHECK(logger.log(ELogLevel::Info, "after"));
    REQUIRE_EQ(logger.lines.size(), std::size_t{1});

    const auto parsed = Logger::parseRecord(logger.lines.front());
    REQUIRE(parsed.has_value());
    CHECK_EQ(parsed->message, "after");
}

TEST(logger_base, propagates_write_failure) {
    RecordingLogger logger{ELogLevel::Debug};
    logger.writeResult = false;

    CHECK(!logger.log(ELogLevel::Info, "doomed"));
    CHECK_EQ(logger.lines.size(), std::size_t{1});

    logger.setLevel(ELogLevel::Fatal);
    CHECK(logger.log(ELogLevel::Info, "filtered"));
    CHECK_EQ(logger.lines.size(), std::size_t{1});
}

TEST(logger_base, multiline_message_stays_one_line) {
    RecordingLogger logger{ELogLevel::Debug};
    CHECK(logger.log(ELogLevel::Info, "first\nsecond"));

    REQUIRE_EQ(logger.lines.size(), std::size_t{1});
    CHECK(logger.lines.front().find('\n') == std::string::npos);

    const auto parsed = Logger::parseRecord(logger.lines.front());
    REQUIRE(parsed.has_value());
    CHECK_EQ(parsed->message, "first\nsecond");
}

TEST(logger_base, accepts_a_view_without_a_terminator) {
    const char raw[] = {'H', 'E', 'L', 'L', 'O'};

    RecordingLogger logger{ELogLevel::Debug};
    CHECK(logger.log(ELogLevel::Info, std::string_view{raw, sizeof(raw)}));

    REQUIRE_EQ(logger.lines.size(), std::size_t{1});
    const auto parsed = Logger::parseRecord(logger.lines.front());
    REQUIRE(parsed.has_value());
    CHECK_EQ(parsed->message, "HELLO");
}

TEST(logger_base, serializes_concurrent_writes) {
    constexpr int kThreads = 4;
    constexpr int kPerThread = 250;

    RecordingLogger logger{ELogLevel::Debug};

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int index = 0; index < kThreads; ++index) {
        workers.emplace_back([&logger, index] {
            const std::string message = "thread " + std::to_string(index);
            for (int i = 0; i < kPerThread; ++i) {
                (void)logger.log(ELogLevel::Info, message);
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    REQUIRE_EQ(logger.lines.size(), static_cast<std::size_t>(kThreads * kPerThread));

    std::size_t parsed = 0;
    for (const auto& line : logger.lines) {
        if (Logger::parseRecord(line).has_value()) ++parsed;
    }
    CHECK_EQ(parsed, logger.lines.size());
}
