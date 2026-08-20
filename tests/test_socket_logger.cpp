#include <framework/TestFramework.hpp>
#include <utils/LoopbackServer.hpp>
#include <utils/TempFile.hpp>

#include <logger/LogLevel.hpp>
#include <logger/LogRecord.hpp>
#include <logger/LoggerFactory.hpp>
#include <net/TcpSink.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

using Logger::ELogLevel;
using Logger::ESocketProtocol;
using utils::LoopbackServer;
using utils::TempFile;

namespace {
    constexpr auto kTimeout = std::chrono::seconds{3};
    const std::string kHost = "127.0.0.1";

    std::uint16_t closedPort() {
        LoopbackServer server{ESocketProtocol::Tcp};
        const auto port = server.port();
        server.stop();
        return port;
    }
} // namespace

TEST(socket_logger, sends_records_over_tcp) {
    LoopbackServer server{ESocketProtocol::Tcp};
    REQUIRE(server.started());

    std::error_code ec;
    const auto logger = Logger::createLogger(
        Logger::SSocketTarget{kHost, server.port(), ESocketProtocol::Tcp}, ELogLevel::Debug, ec
    );
    REQUIRE(logger != nullptr);
    CHECK(!ec);

    CHECK(logger->log(ELogLevel::Info, "first"));
    CHECK(logger->log(ELogLevel::Warn, "second"));

    REQUIRE(server.waitForLines(2, kTimeout));

    const auto lines = server.lines();
    REQUIRE_EQ(lines.size(), std::size_t{2});

    const auto parsed = Logger::parseRecord(lines.front());
    REQUIRE(parsed.has_value());
    CHECK_EQ(parsed->level, ELogLevel::Info);
    CHECK_EQ(parsed->message, "first");

    const auto second = Logger::parseRecord(lines.back());
    REQUIRE(second.has_value());
    CHECK_EQ(second->message, "second");
}

TEST(socket_logger, a_record_survives_a_newline_over_tcp) {
    LoopbackServer server{ESocketProtocol::Tcp};
    REQUIRE(server.started());

    std::error_code ec;
    const auto logger = Logger::createLogger(
        Logger::SSocketTarget{kHost, server.port(), ESocketProtocol::Tcp}, ELogLevel::Debug, ec
    );
    REQUIRE(logger != nullptr);

    CHECK(logger->log(ELogLevel::Info, "first\nsecond"));
    REQUIRE(server.waitForLines(1, kTimeout));

    const auto lines = server.lines();
    REQUIRE_EQ(lines.size(), std::size_t{1});

    const auto parsed = Logger::parseRecord(lines.front());
    REQUIRE(parsed.has_value());
    CHECK_EQ(parsed->message, "first\nsecond");
}

TEST(socket_logger, reports_a_refused_connection) {
    std::error_code ec;
    const auto logger = Logger::createLogger(
        Logger::SSocketTarget{kHost, closedPort(), ESocketProtocol::Tcp}, ELogLevel::Debug, ec
    );

    CHECK(logger == nullptr);
    CHECK(ec == std::errc::connection_refused);
}

TEST(socket_logger, reports_an_unresolvable_host) {
    std::error_code ec;
    const auto logger = Logger::createLogger(
        Logger::SSocketTarget{"no.such.host.invalid", 5555, ESocketProtocol::Tcp},
        ELogLevel::Debug,
        ec
    );

    CHECK(logger == nullptr);
    REQUIRE(static_cast<bool>(ec));
    // getaddrinfo codes overlap errno numerically, so they must not read as errno
    CHECK(ec.category() != std::generic_category());
}

TEST(socket_logger, reconnects_after_the_receiver_drops) {
    LoopbackServer server{ESocketProtocol::Tcp};
    REQUIRE(server.started());

    Logger::STcpSettings settings;
    settings.initialCooldown = std::chrono::milliseconds::zero();
    settings.maxCooldown = std::chrono::milliseconds::zero();

    std::error_code ec;
    const auto sink = Logger::openTcpSink(kHost, server.port(), settings, ec);
    REQUIRE(sink != nullptr);

    CHECK(sink->writeLine("before", ec));
    REQUIRE(server.waitForLines(1, kTimeout));

    server.dropConnection();
    REQUIRE(server.waitForDisconnect(kTimeout));

    bool failed = false;
    for (int attempt = 0; attempt < 50 && !failed; ++attempt) {
        std::error_code writeError;
        failed = !sink->writeLine("lost", writeError);
        if (!failed) std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    REQUIRE(failed);

    std::error_code again;
    CHECK(sink->writeLine("after", again));
    REQUIRE(server.waitForLines(2, kTimeout));

    const auto lines = server.lines();
    CHECK_EQ(lines.front(), "before");
    CHECK_EQ(lines.back(), "after");
}

TEST(socket_logger, sends_one_datagram_per_record_over_udp) {
    LoopbackServer server{ESocketProtocol::Udp};
    REQUIRE(server.started());

    std::error_code ec;
    const auto logger = Logger::createLogger(
        Logger::SSocketTarget{kHost, server.port(), ESocketProtocol::Udp}, ELogLevel::Debug, ec
    );
    REQUIRE(logger != nullptr);

    for (int i = 0; i < 5; ++i) {
        CHECK(logger->log(ELogLevel::Info, "message " + std::to_string(i)));
    }

    REQUIRE(server.waitForLines(5, kTimeout));

    const auto lines = server.lines();
    REQUIRE_EQ(lines.size(), std::size_t{5});
    for (const auto& line : lines) {
        CHECK(Logger::parseRecord(line).has_value());
    }
}

TEST(socket_logger, refuses_an_oversized_datagram) {
    const LoopbackServer server{ESocketProtocol::Udp};
    REQUIRE(server.started());

    std::error_code ec;
    const auto logger = Logger::createLogger(
        Logger::SSocketTarget{kHost, server.port(), ESocketProtocol::Udp}, ELogLevel::Debug, ec
    );
    REQUIRE(logger != nullptr);

    std::error_code writeError;
    CHECK(!logger->log(ELogLevel::Info, std::string(std::size_t{128} * 1024, 'x'), writeError));
    CHECK(static_cast<bool>(writeError));
}

TEST(socket_logger, rejects_an_invalid_protocol) {
    std::error_code ec;
    const auto logger = Logger::createLogger(
        Logger::SSocketTarget{kHost, 5555, static_cast<ESocketProtocol>(42)}, ELogLevel::Debug, ec
    );

    CHECK(logger == nullptr);
    CHECK(ec == std::errc::invalid_argument);
}

TEST(socket_logger, a_file_and_a_socket_get_the_same_record) {
    LoopbackServer server{ESocketProtocol::Tcp};
    REQUIRE(server.started());

    const TempFile file{"socket_logger_multi.log"};

    std::error_code ec;
    const auto logger = Logger::createLogger(
        {Logger::SFileTarget{file.path()},
         Logger::SSocketTarget{kHost, server.port(), ESocketProtocol::Tcp}},
        ELogLevel::Debug,
        ec
    );
    REQUIRE(logger != nullptr);
    CHECK(!ec);

    CHECK(logger->log(ELogLevel::Warn, "to both"));
    REQUIRE(server.waitForLines(1, kTimeout));

    const auto onTheWire = server.lines();
    const auto inTheFile = file.readLines();
    REQUIRE_EQ(onTheWire.size(), std::size_t{1});
    REQUIRE_EQ(inTheFile.size(), std::size_t{1});

    CHECK_EQ(onTheWire.front(), inTheFile.front());
}

TEST(socket_logger, a_failing_target_fails_creation) {
    const TempFile file{"socket_logger_partial.log"};

    LoopbackServer server{ESocketProtocol::Tcp};
    REQUIRE(server.started());
    const auto port = server.port();
    server.stop();

    const std::vector<Logger::TTarget> targets{
        Logger::SFileTarget{file.path()}, Logger::SSocketTarget{kHost, port, ESocketProtocol::Tcp}
    };

    std::error_code ec;
    const auto logger = Logger::createLogger(targets, ELogLevel::Debug, ec);

    CHECK(logger == nullptr);
    CHECK(static_cast<bool>(ec));
}

TEST(socket_logger, rejects_an_empty_target_list) {
    std::error_code ec;
    const auto logger = Logger::createLogger(std::vector<Logger::TTarget>{}, ELogLevel::Debug, ec);

    CHECK(logger == nullptr);
    CHECK(ec == std::errc::invalid_argument);
}
