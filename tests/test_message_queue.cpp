#include <framework/TestFramework.hpp>

#include <core/MessageQueue.hpp>

#include <logger/LogLevel.hpp>

#include <atomic>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>

using App::MessageQueue;
using App::SMessage;
using Logger::ELogLevel;

TEST(message_queue, keeps_the_fifo_order) {
    MessageQueue queue;

    CHECK(queue.push({ELogLevel::Info, "first"}));
    CHECK(queue.push({ELogLevel::Warn, "second"}));
    REQUIRE_EQ(queue.size(), std::size_t{2});

    SMessage message;
    REQUIRE(queue.pop(message));
    CHECK_EQ(message.message, "first");
    CHECK_EQ(message.level, ELogLevel::Info);

    REQUIRE(queue.pop(message));
    CHECK_EQ(message.message, "second");
    CHECK_EQ(message.level, ELogLevel::Warn);
}

TEST(message_queue, pop_waits_for_a_push) {
    MessageQueue queue;
    std::atomic<bool> popped{false};
    SMessage received;

    std::thread consumer([&] {
        if (queue.pop(received)) popped.store(true);
    });

    CHECK(!popped.load());
    queue.push({ELogLevel::Info, "wake up"});
    consumer.join();

    REQUIRE(popped.load());
    CHECK_EQ(received.message, "wake up");
}

TEST(message_queue, close_wakes_a_waiting_consumer) {
    MessageQueue queue;
    std::atomic<bool> result{true};

    std::thread consumer([&] {
        SMessage message;
        result.store(queue.pop(message));
    });

    queue.close();
    consumer.join();

    CHECK(!result.load());
    CHECK(queue.isClosed());
}

TEST(message_queue, close_keeps_what_was_already_accepted) {
    MessageQueue queue;

    CHECK(queue.push({ELogLevel::Info, "first"}));
    CHECK(queue.push({ELogLevel::Info, "second"}));
    queue.close();

    SMessage message;
    REQUIRE(queue.pop(message));
    CHECK_EQ(message.message, "first");

    REQUIRE(queue.pop(message));
    CHECK_EQ(message.message, "second");

    CHECK(!queue.pop(message));
}

TEST(message_queue, push_into_a_closed_queue_is_refused) {
    MessageQueue queue;
    queue.close();

    CHECK(!queue.push({ELogLevel::Info, "too late"}));
    CHECK_EQ(queue.size(), std::size_t{0});
}

TEST(message_queue, keeps_the_order_of_every_producer) {
    constexpr int kProducers = 3;
    constexpr int kPerProducer = 200;

    MessageQueue queue;
    std::vector<SMessage> received;

    std::thread consumer([&] {
        SMessage message;
        while (queue.pop(message)) received.push_back(std::move(message));
    });

    std::vector<std::thread> producers;
    producers.reserve(kProducers);
    for (int id = 0; id < kProducers; ++id) {
        producers.emplace_back([&queue, id] {
            for (int index = 0; index < kPerProducer; ++index) {
                queue.push({
                    ELogLevel::Info,
                    std::to_string(id) + ':' + std::to_string(index)
                });
            }
        });
    }
    for (auto& producer : producers) producer.join();

    queue.close();
    consumer.join();

    REQUIRE_EQ(received.size(), static_cast<std::size_t>(kProducers * kPerProducer));

    std::vector<int> expected(kProducers, 0);
    for (const auto& message : received) {
        const auto colon = message.message.find(':');
        REQUIRE(colon != std::string::npos);

        const int id = std::stoi(message.message.substr(0, colon));
        const int index = std::stoi(message.message.substr(colon + 1));
        REQUIRE(id >= 0 && id < kProducers);

        REQUIRE_EQ(index, expected[static_cast<std::size_t>(id)]);
        expected[static_cast<std::size_t>(id)] = index + 1;
    }
}
