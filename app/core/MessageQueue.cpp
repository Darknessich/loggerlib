#include "MessageQueue.hpp"

#include <utility>

namespace App {
    bool MessageQueue::push(SMessage msg) {
        {
            const std::lock_guard lock(m_mutex);
            if (m_closed) return false;
            m_queue.emplace(std::move(msg));
        }
        m_cv.notify_one();
        return true;
    }

    bool MessageQueue::pop(SMessage& out) {
        std::unique_lock lock(m_mutex);
        m_cv.wait(lock, [this] { return m_closed || !m_queue.empty(); });
        if (m_queue.empty()) return false;
        out = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    void MessageQueue::close() noexcept {
        {
            const std::lock_guard lock(m_mutex);
            if (m_closed) return;
            m_closed = true;
        }
        m_cv.notify_all();
    }

    bool MessageQueue::isClosed() const {
        const std::lock_guard lock(m_mutex);
        return m_closed;
    }

    std::size_t MessageQueue::size() const {
        const std::lock_guard lock(m_mutex);
        return m_queue.size();
    }
} // namespace App
