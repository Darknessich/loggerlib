#include "EventQueue.hpp"

#include <utility>

namespace App {
    bool EventQueue::push(SEvent event) {
        {
            const std::lock_guard lock(m_mutex);
            if (m_closed) return false;
            m_queue.emplace(std::move(event));
        }
        m_cv.notify_one();
        return true;
    }

    bool EventQueue::pop(SEvent& out) {
        std::unique_lock lock(m_mutex);
        m_cv.wait(lock, [this] { return m_closed || !m_queue.empty(); });
        if (m_queue.empty()) return false;
        out = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    void EventQueue::close() noexcept {
        {
            const std::lock_guard lock(m_mutex);
            if (m_closed) return;
            m_closed = true;
        }
        m_cv.notify_all();
    }

    bool EventQueue::isClosed() const {
        const std::lock_guard lock(m_mutex);
        return m_closed;
    }

    std::size_t EventQueue::size() const {
        const std::lock_guard lock(m_mutex);
        return m_queue.size();
    }
} // namespace App
