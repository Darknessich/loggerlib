#pragma once

#include <logger/LogLevel.hpp>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <variant>

namespace App {
    struct SWrite {
        Logger::ELogLevel level{Logger::ELogLevel::Info};
        std::string message;
    };

    struct SSetLevel {
        Logger::ELogLevel level{Logger::ELogLevel::Info};
    };

    using TEvent = std::variant<SWrite, SSetLevel>;

    // close() refuses new events but keeps the queued ones
    class EventQueue {
    public:
        EventQueue() = default;
        ~EventQueue() = default;

        EventQueue(const EventQueue&) = delete;
        EventQueue& operator=(const EventQueue&) = delete;
        EventQueue(EventQueue&&) = delete;
        EventQueue& operator=(EventQueue&&) = delete;

        [[nodiscard]] bool push(TEvent event);
        bool pop(TEvent& out);
        void close() noexcept;

    private:
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
        std::queue<TEvent> m_queue;
        bool m_closed{false};
    };
} // namespace App
