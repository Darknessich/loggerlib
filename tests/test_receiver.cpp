#include <framework/TestFramework.hpp>

#include <core/net/Receiver.hpp>

#include <common/net/Address.hpp>
#include <common/net/Socket.hpp>

#include <logger/LogLevel.hpp>
#include <logger/LogRecord.hpp>
#include <logger/LoggerFactory.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <sys/socket.h>

using Collector::IReceiver;
using Collector::openReceiver;
using Logger::ELogLevel;
using Logger::ESocketProtocol;

namespace {
    constexpr auto kStep = std::chrono::milliseconds{50};
    constexpr int kAttempts = 60;
    const std::string kHost = "127.0.0.1";

    bool collect(IReceiver& receiver, std::size_t count, std::vector<std::string>& lines) {
        std::error_code ec;
        for (int attempt = 0; attempt < kAttempts && lines.size() < count; ++attempt) {
            if (!receiver.receive(lines, kStep, ec)) return false;
        }
        return lines.size() >= count;
    }

    void pump(IReceiver& receiver, std::vector<std::string>& lines, int rounds) {
        std::error_code ec;
        for (int round = 0; round < rounds; ++round) {
            (void)receiver.receive(lines, kStep, ec);
        }
    }

    Common::Socket connectTo(std::uint16_t port) {
        std::error_code ec;
        const auto endpoints = Common::resolve(kHost, port, SOCK_STREAM, ec);
        if (ec || endpoints.empty()) return Common::Socket{};

        const auto& endpoint = endpoints.front();
        Common::Socket socket{::socket(endpoint.family, endpoint.socktype, endpoint.protocol)};
        if (!socket.valid()) return Common::Socket{};
        if (::connect(socket.get(), Common::asSockaddr(endpoint), endpoint.length) != 0)
            return Common::Socket{};

        return socket;
    }

    bool send(const Common::Socket& socket, std::string_view text) {
        return ::send(socket.get(), text.data(), text.size(), 0) ==
               static_cast<ssize_t>(text.size());
    }

    std::unique_ptr<Logger::ILogger>
    loggerFor(const IReceiver& receiver, ESocketProtocol protocol) {
        std::error_code ec;
        return Logger::createLogger(
            Logger::SSocketTarget{kHost, receiver.port(), protocol}, ELogLevel::Debug, ec
        );
    }
} // namespace

TEST(receiver, takes_records_over_tcp) {
    std::error_code ec;
    const auto receiver = openReceiver(kHost, 0, ESocketProtocol::Tcp, ec);
    REQUIRE(receiver != nullptr);
    CHECK(!ec);

    const auto logger = loggerFor(*receiver, ESocketProtocol::Tcp);
    REQUIRE(logger != nullptr);

    CHECK(logger->log(ELogLevel::Info, "first"));
    CHECK(logger->log(ELogLevel::Warn, "second"));

    std::vector<std::string> lines;
    REQUIRE(collect(*receiver, 2, lines));
    REQUIRE_EQ(lines.size(), std::size_t{2});

    const auto first = Logger::parseRecord(lines.front());
    REQUIRE(first.has_value());
    CHECK_EQ(first->level, ELogLevel::Info);
    CHECK_EQ(first->message, "first");

    const auto second = Logger::parseRecord(lines.back());
    REQUIRE(second.has_value());
    CHECK_EQ(second->message, "second");
}

TEST(receiver, takes_a_sender_that_comes_back) {
    std::error_code ec;
    const auto receiver = openReceiver(kHost, 0, ESocketProtocol::Tcp, ec);
    REQUIRE(receiver != nullptr);

    std::vector<std::string> lines;
    for (int round = 0; round < 2; ++round) {
        const auto logger = loggerFor(*receiver, ESocketProtocol::Tcp);
        REQUIRE(logger != nullptr);
        CHECK(logger->log(ELogLevel::Info, "round " + std::to_string(round)));

        REQUIRE(collect(*receiver, static_cast<std::size_t>(round) + 1, lines));
    }

    REQUIRE_EQ(lines.size(), std::size_t{2});
    CHECK(lines.front().find("round 0") != std::string::npos);
    CHECK(lines.back().find("round 1") != std::string::npos);
}

TEST(receiver, keeps_two_senders_apart) {
    std::error_code ec;
    const auto receiver = openReceiver(kHost, 0, ESocketProtocol::Tcp, ec);
    REQUIRE(receiver != nullptr);

    const auto one = connectTo(receiver->port());
    const auto other = connectTo(receiver->port());
    REQUIRE(one.valid());
    REQUIRE(other.valid());

    std::vector<std::string> lines;
    CHECK(send(one, "half of one"));

    pump(*receiver, lines, 3);
    REQUIRE(lines.empty());

    CHECK(send(other, "all of the other\n"));
    CHECK(send(one, " and the rest\n"));

    REQUIRE(collect(*receiver, 2, lines));
    REQUIRE_EQ(lines.size(), std::size_t{2});

    CHECK(std::find(lines.begin(), lines.end(), "all of the other") != lines.end());
    CHECK(std::find(lines.begin(), lines.end(), "half of one and the rest") != lines.end());
}

TEST(receiver, takes_datagrams_over_udp) {
    std::error_code ec;
    const auto receiver = openReceiver(kHost, 0, ESocketProtocol::Udp, ec);
    REQUIRE(receiver != nullptr);

    const auto logger = loggerFor(*receiver, ESocketProtocol::Udp);
    REQUIRE(logger != nullptr);

    for (int i = 0; i < 5; ++i) {
        CHECK(logger->log(ELogLevel::Info, "message " + std::to_string(i)));
    }

    std::vector<std::string> lines;
    REQUIRE(collect(*receiver, 5, lines));
    REQUIRE_EQ(lines.size(), std::size_t{5});

    std::vector<std::string> messages;
    for (const auto& line : lines) {
        const auto record = Logger::parseRecord(line);
        REQUIRE(record.has_value());
        messages.push_back(record->message);
    }

    std::sort(messages.begin(), messages.end());
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(messages[static_cast<std::size_t>(i)], "message " + std::to_string(i));
    }
}

TEST(receiver, an_empty_host_listens_everywhere) {
    std::error_code ec;
    const auto receiver = openReceiver("", 0, ESocketProtocol::Tcp, ec);

    REQUIRE(receiver != nullptr);
    CHECK(!ec);
    CHECK(receiver->port() != 0);

    CHECK(receiver->address() == "0.0.0.0" || receiver->address() == "::");
}

TEST(receiver, a_named_host_is_bound_as_asked) {
    std::error_code ec;
    const auto receiver = openReceiver(kHost, 0, ESocketProtocol::Tcp, ec);

    REQUIRE(receiver != nullptr);
    CHECK_EQ(receiver->address(), kHost);
}

TEST(receiver, reports_a_port_already_taken) {
    std::error_code ec;
    const auto first = openReceiver(kHost, 0, ESocketProtocol::Tcp, ec);
    REQUIRE(first != nullptr);

    const auto second = openReceiver(kHost, first->port(), ESocketProtocol::Tcp, ec);

    CHECK(second == nullptr);
    CHECK(ec == std::errc::address_in_use);
}

TEST(receiver, rejects_an_invalid_protocol) {
    std::error_code ec;
    const auto receiver = openReceiver(kHost, 0, static_cast<ESocketProtocol>(42), ec);

    CHECK(receiver == nullptr);
    CHECK(ec == std::errc::invalid_argument);
}
