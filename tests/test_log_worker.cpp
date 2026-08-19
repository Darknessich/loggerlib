#include <framework/TestFramework.hpp>
#include <utils/RecordingLogger.hpp>

#include <core/LogWorker.hpp>
#include <core/MessageQueue.hpp>

#include <logger/LogLevel.hpp>

#include <cstddef>
#include <string>
#include <system_error>

using App::LogWorker;
using App::MessageQueue;
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
    MessageQueue queue;
    LogWorker worker{logger, queue};

    for (std::size_t i = 0; i < kCount; ++i) {
        REQUIRE(queue.push({ELogLevel::Info, "message " + std::to_string(i)}));
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

    MessageQueue queue;
    LogWorker worker{logger, queue};

    for (std::size_t i = 0; i < kCount; ++i) {
        REQUIRE(queue.push({ELogLevel::Error, "doomed"}));
    }

    worker.start();
    worker.stop();

    REQUIRE_EQ(worker.failed(), kCount);
    CHECK_EQ(worker.processed(), std::size_t{0});
    CHECK_EQ(logger.count(), kCount);
}

TEST(log_worker, counts_a_filtered_message_as_processed) {
    RecordingLogger logger{ELogLevel::Warn};
    MessageQueue queue;
    LogWorker worker{logger, queue};

    REQUIRE(queue.push({ELogLevel::Debug, "below the threshold"}));

    worker.start();
    worker.stop();

    REQUIRE_EQ(worker.processed(), std::size_t{1});
    CHECK_EQ(worker.failed(), std::size_t{0});
    CHECK_EQ(logger.count(), std::size_t{0});
}

TEST(log_worker, last_error_is_empty_without_failures) {
    RecordingLogger logger{ELogLevel::Debug};
    MessageQueue queue;
    LogWorker worker{logger, queue};

    REQUIRE(queue.push({ELogLevel::Info, "fine"}));
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

    MessageQueue queue;
    LogWorker worker{logger, queue};
    REQUIRE(queue.push({ELogLevel::Info, "first"}));
    REQUIRE(queue.push({ELogLevel::Info, "second"}));

    worker.start();
    worker.stop();

    REQUIRE_EQ(worker.failed(), std::size_t{2});
    CHECK_EQ(worker.lastError(), std::make_error_code(std::errc::broken_pipe).message());
}

TEST(log_worker, carries_an_error_from_a_foreign_category) {
    RecordingLogger logger{ELogLevel::Debug};
    logger.failWrites(std::error_code{7, testCategory()});

    MessageQueue queue;
    LogWorker worker{logger, queue};
    REQUIRE(queue.push({ELogLevel::Info, "rejected"}));

    worker.start();
    worker.stop();

    REQUIRE_EQ(worker.failed(), std::size_t{1});
    CHECK_EQ(worker.lastError(), "schema rejected the record (7)");
}

TEST(log_worker, survives_an_exception_from_the_logger) {
    RecordingLogger logger{ELogLevel::Debug};
    logger.throwOnWrite();

    MessageQueue queue;
    LogWorker worker{logger, queue};
    REQUIRE(queue.push({ELogLevel::Info, "boom"}));

    worker.start();
    worker.stop();

    REQUIRE_EQ(worker.failed(), std::size_t{1});
    CHECK_EQ(worker.processed(), std::size_t{0});
    CHECK(worker.lastError().find("exploded") != std::string::npos);
}

TEST(log_worker, stop_is_idempotent) {
    RecordingLogger logger{ELogLevel::Debug};
    MessageQueue queue;
    LogWorker worker{logger, queue};

    REQUIRE(queue.push({ELogLevel::Info, "only one"}));

    worker.start();
    worker.stop();
    worker.stop();

    REQUIRE_EQ(logger.count(), std::size_t{1});
}

TEST(log_worker, a_worker_that_never_started_can_be_destroyed) {
    RecordingLogger logger{ELogLevel::Debug};
    MessageQueue queue;

    {
        const LogWorker worker{logger, queue};
        CHECK_EQ(worker.processed(), std::size_t{0});
    }

    CHECK(queue.isClosed());
}
