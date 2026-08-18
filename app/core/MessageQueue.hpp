#pragma once

#include <logger/LogLevel.hpp>

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <string>

namespace App {
    struct SMessage {
        Logger::ELogLevel level{Logger::ELogLevel::Info};
        std::string message;
    };

    class MessageQueue {
    public:
        MessageQueue() = default;
        ~MessageQueue() = default;

        MessageQueue(const MessageQueue&) = delete;
        MessageQueue& operator=(const MessageQueue&) = delete;
        MessageQueue(MessageQueue&&) = delete;
        MessageQueue& operator=(MessageQueue&&) = delete;

        [[nodiscard]] bool push(SMessage msg);
        bool pop(SMessage& out);
        void close() noexcept;
        bool isClosed() const noexcept;
        std::size_t size() const noexcept;

    private:
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
        std::queue<SMessage> m_queue;
        bool m_closed{false};
    };
} // namespace App
