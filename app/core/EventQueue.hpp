#pragma once

#include <logger/LogLevel.hpp>

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <string>

namespace App {
    enum class EEventKind { Write, SetLevel };

    struct SEvent {
        EEventKind kind{EEventKind::Write};
        Logger::ELogLevel level{Logger::ELogLevel::Info};
        std::string message;
    };

    class EventQueue {
    public:
        EventQueue() = default;
        ~EventQueue() = default;

        EventQueue(const EventQueue&) = delete;
        EventQueue& operator=(const EventQueue&) = delete;
        EventQueue(EventQueue&&) = delete;
        EventQueue& operator=(EventQueue&&) = delete;

        [[nodiscard]] bool push(SEvent event);
        bool pop(SEvent& out);
        void close() noexcept;

    private:
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
        std::queue<SEvent> m_queue;
        bool m_closed{false};
    };
} // namespace App
