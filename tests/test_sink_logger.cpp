#include <framework/TestFramework.hpp>

#include <ISink.hpp>
#include <SinkLogger.hpp>
#include <logger/LogLevel.hpp>
#include <logger/LogRecord.hpp>

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

using Logger::ELogLevel;

namespace {
    struct SRecordingSink {
        std::vector<std::string> lines;
        bool writeResult = true;
        std::error_code writeError = std::make_error_code(std::errc::io_error);
    };

    class SinkProxy final : public Logger::ISink {
    public:
        explicit SinkProxy(SRecordingSink& target) noexcept : m_target{&target} {}

        bool writeLine(std::string_view line, std::error_code& ec) override {
            m_target->lines.emplace_back(line);
            if (m_target->writeResult) return true;
            ec = m_target->writeError;
            return false;
        }

    private:
        SRecordingSink* m_target;
    };

    class TestLogger final : public Logger::SinkLogger {
    public:
        TestLogger(SRecordingSink& sink, ELogLevel level)
            : SinkLogger{std::make_unique<SinkProxy>(sink), level} {}

        static std::chrono::system_clock::time_point fixedTime() {
            return std::chrono::system_clock::from_time_t(0);
        }

    protected:
        [[nodiscard]] std::chrono::system_clock::time_point now() const noexcept override {
            return fixedTime();
        }
    };
} // namespace

TEST(sink_logger, writes_at_or_above_the_threshold) {
    SRecordingSink sink;
    TestLogger logger{sink, ELogLevel::Warn};

    CHECK(logger.log(ELogLevel::Warn, "warn"));
    CHECK(logger.log(ELogLevel::Error, "error"));
    CHECK(logger.log(ELogLevel::Fatal, "fatal"));

    CHECK_EQ(sink.lines.size(), std::size_t{3});
}

TEST(sink_logger, drops_messages_below_the_threshold) {
    SRecordingSink sink;
    TestLogger logger{sink, ELogLevel::Warn};

    CHECK(logger.log(ELogLevel::Debug, "debug"));
    CHECK(logger.log(ELogLevel::Info, "info"));

    CHECK_EQ(sink.lines.size(), std::size_t{0});
}

TEST(sink_logger, line_matches_format_record) {
    SRecordingSink sink;
    TestLogger logger{sink, ELogLevel::Debug};
    CHECK(logger.log(ELogLevel::Info, "hello"));

    REQUIRE_EQ(sink.lines.size(), std::size_t{1});
    CHECK_EQ(
        sink.lines.front(), Logger::formatRecord(TestLogger::fixedTime(), ELogLevel::Info, "hello")
    );
}

TEST(sink_logger, set_level_takes_effect_immediately) {
    SRecordingSink sink;
    TestLogger logger{sink, ELogLevel::Warn};

    CHECK(logger.log(ELogLevel::Info, "before"));
    CHECK_EQ(sink.lines.size(), std::size_t{0});

    logger.setLevel(ELogLevel::Debug);
    CHECK_EQ(logger.level(), ELogLevel::Debug);

    CHECK(logger.log(ELogLevel::Info, "after"));
    REQUIRE_EQ(sink.lines.size(), std::size_t{1});

    const auto parsed = Logger::parseRecord(sink.lines.front());
    REQUIRE(parsed.has_value());
    CHECK_EQ(parsed->message, "after");
}

TEST(sink_logger, propagates_write_failure) {
    SRecordingSink sink;
    sink.writeResult = false;
    TestLogger logger{sink, ELogLevel::Debug};

    CHECK(!logger.log(ELogLevel::Info, "doomed"));
    CHECK_EQ(sink.lines.size(), std::size_t{1});

    logger.setLevel(ELogLevel::Fatal);
    CHECK(logger.log(ELogLevel::Info, "filtered"));
    CHECK_EQ(sink.lines.size(), std::size_t{1});
}

TEST(sink_logger, error_code_is_clear_on_success) {
    SRecordingSink sink;
    TestLogger logger{sink, ELogLevel::Warn};

    std::error_code ec = std::make_error_code(std::errc::io_error);
    CHECK(logger.log(ELogLevel::Error, "written", ec));
    CHECK(!ec);

    ec = std::make_error_code(std::errc::io_error);
    CHECK(logger.log(ELogLevel::Debug, "filtered", ec));
    CHECK(!ec);
}

TEST(sink_logger, passes_the_error_code_through) {
    SRecordingSink sink;
    sink.writeResult = false;
    sink.writeError = std::make_error_code(std::errc::no_space_on_device);
    TestLogger logger{sink, ELogLevel::Debug};

    std::error_code ec;
    CHECK(!logger.log(ELogLevel::Info, "doomed", ec));
    CHECK(ec == std::errc::no_space_on_device);
}

TEST(sink_logger, rejects_an_invalid_level) {
    SRecordingSink sink;
    TestLogger logger{sink, ELogLevel::Debug};

    for (const ELogLevel level :
         {ELogLevel::Count,
          static_cast<ELogLevel>(42),
          static_cast<ELogLevel>(255),
          static_cast<ELogLevel>(-1)}) {
        std::error_code ec;
        CHECK(!logger.log(level, "garbage", ec));
        CHECK(ec == std::errc::invalid_argument);
    }

    CHECK_EQ(sink.lines.size(), std::size_t{0});
}

TEST(sink_logger, multiline_message_stays_one_line) {
    SRecordingSink sink;
    TestLogger logger{sink, ELogLevel::Debug};
    CHECK(logger.log(ELogLevel::Info, "first\nsecond"));

    REQUIRE_EQ(sink.lines.size(), std::size_t{1});
    CHECK(sink.lines.front().find('\n') == std::string::npos);

    const auto parsed = Logger::parseRecord(sink.lines.front());
    REQUIRE(parsed.has_value());
    CHECK_EQ(parsed->message, "first\nsecond");
}

TEST(sink_logger, accepts_a_view_without_a_terminator) {
    const char raw[] = {'H', 'E', 'L', 'L', 'O'};

    SRecordingSink sink;
    TestLogger logger{sink, ELogLevel::Debug};
    CHECK(logger.log(ELogLevel::Info, std::string_view{raw, sizeof(raw)}));

    REQUIRE_EQ(sink.lines.size(), std::size_t{1});
    const auto parsed = Logger::parseRecord(sink.lines.front());
    REQUIRE(parsed.has_value());
    CHECK_EQ(parsed->message, "HELLO");
}

TEST(sink_logger, serializes_concurrent_writes) {
    constexpr int kThreads = 4;
    constexpr int kPerThread = 250;

    SRecordingSink sink;
    TestLogger logger{sink, ELogLevel::Debug};

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

    REQUIRE_EQ(sink.lines.size(), static_cast<std::size_t>(kThreads * kPerThread));

    std::size_t parsed = 0;
    for (const auto& line : sink.lines) {
        if (Logger::parseRecord(line).has_value()) ++parsed;
    }
    CHECK_EQ(parsed, sink.lines.size());
}
