#include <framework/TestFramework.hpp>
#include <utils/TempFile.hpp>

#include <logger/LogLevel.hpp>
#include <logger/LogRecord.hpp>
#include <logger/LoggerFactory.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

using utils::TempFile;
using Logger::ELogLevel;

TEST(file_logger, creates_the_file_and_writes_a_record) {
    const TempFile file{"file_logger_creates.log"};

    std::error_code ec;
    const auto logger = Logger::createFileLogger(file.path(), ELogLevel::Info, ec);
    REQUIRE(logger != nullptr);
    CHECK(!ec);

    CHECK(logger->log(ELogLevel::Warn, "first\nsecond"));

    const auto lines = file.readLines();
    REQUIRE_EQ(lines.size(), std::size_t{1});

    const auto parsed = Logger::parseRecord(lines.front());
    REQUIRE(parsed.has_value());
    CHECK_EQ(parsed->level, ELogLevel::Warn);
    CHECK_EQ(parsed->message, "first\nsecond");
}

TEST(file_logger, appends_to_an_existing_file) {
    const TempFile file{"file_logger_appends.log"};

    for (int attempt = 0; attempt < 2; ++attempt) {
        std::error_code ec;
        const auto logger = Logger::createFileLogger(file.path(), ELogLevel::Debug, ec);
        REQUIRE(logger != nullptr);
        CHECK(logger->log(ELogLevel::Info, "line " + std::to_string(attempt)));
    }

    CHECK_EQ(file.readLines().size(), std::size_t{2});
}

TEST(file_logger, respects_the_level) {
    const TempFile file{"file_logger_level.log"};

    std::error_code ec;
    const auto logger = Logger::createFileLogger(file.path(), ELogLevel::Warn, ec);
    REQUIRE(logger != nullptr);

    CHECK(logger->log(ELogLevel::Debug, "dropped"));
    CHECK(logger->log(ELogLevel::Info, "dropped"));
    CHECK_EQ(file.readLines().size(), std::size_t{0});

    CHECK(logger->log(ELogLevel::Error, "kept"));
    CHECK_EQ(file.readLines().size(), std::size_t{1});
}

TEST(file_logger, set_level_takes_effect_immediately) {
    const TempFile file{"file_logger_setlevel.log"};

    std::error_code ec;
    const auto logger = Logger::createFileLogger(file.path(), ELogLevel::Error, ec);
    REQUIRE(logger != nullptr);

    CHECK(logger->log(ELogLevel::Info, "before"));
    CHECK_EQ(file.readLines().size(), std::size_t{0});

    logger->setLevel(ELogLevel::Debug);
    CHECK_EQ(logger->level(), ELogLevel::Debug);

    CHECK(logger->log(ELogLevel::Info, "after"));

    const auto lines = file.readLines();
    REQUIRE_EQ(lines.size(), std::size_t{1});

    const auto parsed = Logger::parseRecord(lines.front());
    REQUIRE(parsed.has_value());
    CHECK_EQ(parsed->message, "after");
}

TEST(file_logger, reports_open_failure) {
    std::error_code ec;
    const auto logger = Logger::createFileLogger("no-such-dir/logger.log", ELogLevel::Info, ec);

    CHECK(logger == nullptr);
    CHECK(static_cast<bool>(ec));
}

TEST(file_logger, rejects_an_invalid_level) {
    const TempFile file{"file_logger_invalid_level.log"};

    for (const ELogLevel level : {ELogLevel::Count, static_cast<ELogLevel>(42)}) {
        std::error_code ec;
        const auto logger = Logger::createFileLogger(file.path(), level, ec);

        CHECK(logger == nullptr);
        CHECK(ec == std::errc::invalid_argument);
    }

    CHECK(!file.exists());
}

TEST(file_logger, serializes_concurrent_writes) {
    constexpr int kThreads = 4;
    constexpr int kPerThread = 250;

    const TempFile file{"file_logger_threads.log"};

    std::error_code ec;
    const auto logger = Logger::createFileLogger(file.path(), ELogLevel::Debug, ec);
    REQUIRE(logger != nullptr);

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int index = 0; index < kThreads; ++index) {
        workers.emplace_back([&logger, index] {
            const std::string message = "thread " + std::to_string(index);
            for (int i = 0; i < kPerThread; ++i) {
                logger->log(ELogLevel::Info, message);
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    const auto lines = file.readLines();
    REQUIRE_EQ(lines.size(), static_cast<std::size_t>(kThreads * kPerThread));

    std::size_t parsed = 0;
    for (const auto& line : lines) {
        if (Logger::parseRecord(line).has_value()) ++parsed;
    }
    CHECK_EQ(parsed, lines.size());
}
