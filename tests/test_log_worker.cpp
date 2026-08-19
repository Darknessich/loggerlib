#include <framework/TestFramework.hpp>
#include <utils/RecordingLogger.hpp>

#include <core/EventQueue.hpp>
#include <core/LogWorker.hpp>

#include <logger/LogLevel.hpp>

#include <cstddef>
#include <string>
#include <system_error>

using App::EEventKind;
using App::EventQueue;
using App::LogWorker;
using Logger::ELogLevel;
using utils::RecordingLogger;

namespace {
    class TestCategory final : public std::error_category {
    public:
        [[nodiscard]] const char* name() const noexcept override { return "test"; }

        [[nodiscard]] std::string message(int value) const override {
            return "schema rejected the record (" + std::to_string(value) + ')';
        }
    };

    const std::error_category& testCategory() {
        static const TestCategory category;
        return category;
    }
} // namespace

TEST(log_worker, writes_everything_queued_before_stop) {
    constexpr std::size_t kCount = 100;

    RecordingLogger logger{ELogLevel::Debug};
    EventQueue queue;
    LogWorker worker{logger, queue};

    for (std::size_t i = 0; i < kCount; ++i) {
        REQUIRE(queue.push({EEventKind::Write, ELogLevel::Info, "message " + std::to_string(i)}));
    }

    worker.start();
    worker.stop();

    REQUIRE_EQ(logger.count(), kCount);
    CHECK_EQ(worker.processed(), kCount);
    CHECK_EQ(worker.failed(), std::size_t{0});

    const auto records = logger.records();
    REQUIRE_EQ(records.size(), kCount);
    CHECK_EQ(records.front().message, "message 0");
    CHECK_EQ(records.back().message, "message 99");
}

TEST(log_worker, counts_failed_writes) {
    constexpr std::size_t kCount = 5;

    RecordingLogger logger{ELogLevel::Debug};
    logger.failWrites();

    EventQueue queue;
    LogWorker worker{logger, queue};

    for (std::size_t i = 0; i < kCount; ++i) {
        REQUIRE(queue.push({EEventKind::Write, ELogLevel::Error, "doomed"}));
    }

    worker.start();
    worker.stop();

    REQUIRE_EQ(worker.failed(), kCount);
    CHECK_EQ(worker.processed(), std::size_t{0});
    CHECK_EQ(logger.count(), kCount);
}

TEST(log_worker, does_not_count_a_filtered_message) {
    RecordingLogger logger{ELogLevel::Warn};
    EventQueue queue;
    LogWorker worker{logger, queue};

    REQUIRE(queue.push({EEventKind::Write, ELogLevel::Debug, "below the threshold"}));

    worker.start();
    worker.stop();

    REQUIRE_EQ(worker.processed(), std::size_t{0});
    CHECK_EQ(worker.failed(), std::size_t{0});
    CHECK_EQ(logger.count(), std::size_t{0});
}

TEST(log_worker, applies_a_level_event_in_queue_order) {
    RecordingLogger logger{ELogLevel::Debug};
    EventQueue queue;
    LogWorker worker{logger, queue};

    REQUIRE(queue.push({EEventKind::Write, ELogLevel::Info, "before"}));
    REQUIRE(queue.push({EEventKind::SetLevel, ELogLevel::Error, {}}));
    REQUIRE(queue.push({EEventKind::Write, ELogLevel::Info, "after"}));

    worker.start();
    worker.stop();

    CHECK_EQ(logger.level(), ELogLevel::Error);
    CHECK_EQ(worker.processed(), std::size_t{1});

    const auto records = logger.records();
    REQUIRE_EQ(records.size(), std::size_t{1});
    CHECK_EQ(records.front().message, "before");
}

TEST(log_worker, last_error_is_empty_without_failures) {
    RecordingLogger logger{ELogLevel::Debug};
    EventQueue queue;
    LogWorker worker{logger, queue};

    REQUIRE(queue.push({EEventKind::Write, ELogLevel::Info, "fine"}));
    worker.start();
    worker.stop();

    CHECK_EQ(worker.failed(), std::size_t{0});
    CHECK(worker.lastError().empty());
}

TEST(log_worker, last_error_follows_the_latest_failure) {
    RecordingLogger logger{ELogLevel::Debug};
    logger.failWrites(
        {std::make_error_code(std::errc::no_space_on_device),
         std::make_error_code(std::errc::broken_pipe)}
    );

    EventQueue queue;
    LogWorker worker{logger, queue};
    REQUIRE(queue.push({EEventKind::Write, ELogLevel::Info, "first"}));
    REQUIRE(queue.push({EEventKind::Write, ELogLevel::Info, "second"}));

    worker.start();
    worker.stop();

    REQUIRE_EQ(worker.failed(), std::size_t{2});
    CHECK_EQ(worker.lastError(), std::make_error_code(std::errc::broken_pipe).message());
}

TEST(log_worker, carries_an_error_from_a_foreign_category) {
    RecordingLogger logger{ELogLevel::Debug};
    logger.failWrites(std::error_code{7, testCategory()});

    EventQueue queue;
    LogWorker worker{logger, queue};
    REQUIRE(queue.push({EEventKind::Write, ELogLevel::Info, "rejected"}));

    worker.start();
    worker.stop();

    REQUIRE_EQ(worker.failed(), std::size_t{1});
    CHECK_EQ(worker.lastError(), "schema rejected the record (7)");
}

TEST(log_worker, survives_an_exception_from_the_logger) {
    RecordingLogger logger{ELogLevel::Debug};
    logger.throwOnWrite();

    EventQueue queue;
    LogWorker worker{logger, queue};
    REQUIRE(queue.push({EEventKind::Write, ELogLevel::Info, "boom"}));

    worker.start();
    worker.stop();

    REQUIRE_EQ(worker.failed(), std::size_t{1});
    CHECK_EQ(worker.processed(), std::size_t{0});
    CHECK(worker.lastError().find("exploded") != std::string::npos);
}

TEST(log_worker, stop_is_idempotent) {
    RecordingLogger logger{ELogLevel::Debug};
    EventQueue queue;
    LogWorker worker{logger, queue};

    REQUIRE(queue.push({EEventKind::Write, ELogLevel::Info, "only one"}));

    worker.start();
    worker.stop();
    worker.stop();

    REQUIRE_EQ(logger.count(), std::size_t{1});
}

TEST(log_worker, a_worker_that_never_started_can_be_destroyed) {
    RecordingLogger logger{ELogLevel::Debug};
    EventQueue queue;

    {
        const LogWorker worker{logger, queue};
        CHECK_EQ(worker.processed(), std::size_t{0});
    }

    CHECK(queue.isClosed());
}
