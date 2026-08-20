#include <framework/TestFramework.hpp>

#include <core/EventQueue.hpp>

#include <logger/LogLevel.hpp>

#include <atomic>
#include <cstddef>
#include <string>
#include <thread>
#include <variant>
#include <vector>

using App::EventQueue;
using App::SWrite;
using App::TEvent;
using Logger::ELogLevel;

TEST(event_queue, keeps_the_fifo_order) {
    EventQueue queue;

    CHECK(queue.push(SWrite{ELogLevel::Info, "first"}));
    CHECK(queue.push(SWrite{ELogLevel::Warn, "second"}));

    TEvent event;
    REQUIRE(queue.pop(event));
    REQUIRE(std::holds_alternative<SWrite>(event));
    CHECK_EQ(std::get<SWrite>(event).message, "first");
    CHECK_EQ(std::get<SWrite>(event).level, ELogLevel::Info);

    REQUIRE(queue.pop(event));
    REQUIRE(std::holds_alternative<SWrite>(event));
    CHECK_EQ(std::get<SWrite>(event).message, "second");
    CHECK_EQ(std::get<SWrite>(event).level, ELogLevel::Warn);
}

TEST(event_queue, pop_waits_for_a_push) {
    EventQueue queue;
    std::atomic<bool> popped{false};
    TEvent received;

    std::thread consumer([&] {
        if (queue.pop(received)) popped.store(true);
    });

    CHECK(!popped.load());
    CHECK(queue.push(SWrite{ELogLevel::Info, "wake up"}));
    consumer.join();

    REQUIRE(popped.load());
    REQUIRE(std::holds_alternative<SWrite>(received));
    CHECK_EQ(std::get<SWrite>(received).message, "wake up");
}

TEST(event_queue, close_wakes_a_waiting_consumer) {
    EventQueue queue;
    std::atomic<bool> result{true};

    std::thread consumer([&] {
        TEvent event;
        result.store(queue.pop(event));
    });

    queue.close();
    consumer.join();

    CHECK(!result.load());
}

TEST(event_queue, close_keeps_accepted_events) {
    EventQueue queue;

    CHECK(queue.push(SWrite{ELogLevel::Info, "first"}));
    CHECK(queue.push(SWrite{ELogLevel::Info, "second"}));
    queue.close();

    TEvent event;
    REQUIRE(queue.pop(event));
    CHECK_EQ(std::get<SWrite>(event).message, "first");

    REQUIRE(queue.pop(event));
    CHECK_EQ(std::get<SWrite>(event).message, "second");

    CHECK(!queue.pop(event));
}

TEST(event_queue, refuses_a_push_after_close) {
    EventQueue queue;
    queue.close();

    CHECK(!queue.push(SWrite{ELogLevel::Info, "too late"}));

    TEvent event;
    CHECK(!queue.pop(event));
}

TEST(event_queue, keeps_the_order_of_every_producer) {
    constexpr int kProducers = 3;
    constexpr int kPerProducer = 200;

    EventQueue queue;
    std::vector<TEvent> received;

    std::thread consumer([&] {
        TEvent event;
        while (queue.pop(event))
            received.push_back(std::move(event));
    });

    std::vector<std::thread> producers;
    producers.reserve(kProducers);
    for (int id = 0; id < kProducers; ++id) {
        producers.emplace_back([&queue, id] {
            for (int index = 0; index < kPerProducer; ++index) {
                std::string text = std::to_string(id) + ':' + std::to_string(index);
                (void)queue.push(SWrite{ELogLevel::Info, std::move(text)});
            }
        });
    }
    for (auto& producer : producers)
        producer.join();

    queue.close();
    consumer.join();

    REQUIRE_EQ(received.size(), static_cast<std::size_t>(kProducers * kPerProducer));

    std::vector<int> expected(kProducers, 0);
    for (const auto& event : received) {
        REQUIRE(std::holds_alternative<SWrite>(event));
        const std::string& message = std::get<SWrite>(event).message;

        const auto colon = message.find(':');
        REQUIRE(colon != std::string::npos);

        const int id = std::stoi(message.substr(0, colon));
        const int index = std::stoi(message.substr(colon + 1));
        REQUIRE(id >= 0 && id < kProducers);

        REQUIRE_EQ(index, expected[static_cast<std::size_t>(id)]);
        expected[static_cast<std::size_t>(id)] = index + 1;
    }
}
