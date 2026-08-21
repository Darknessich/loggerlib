#include <common/net/Socket.hpp>

namespace Common {
    Socket::Socket(int fd) noexcept : m_fd{fd} {}

    Socket::~Socket() {
        reset();
    }

    Socket::Socket(Socket&& other) noexcept : m_fd{other.release()} {}

    Socket& Socket::operator=(Socket&& other) noexcept {
        if (this == &other) return *this;
        reset();
        m_fd = other.release();
        return *this;
    }

    bool Socket::valid() const noexcept {
        return m_fd >= 0;
    }

    int Socket::get() const noexcept {
        return m_fd;
    }

    int Socket::release() noexcept {
        const int fd = m_fd;
        m_fd = -1;
        return fd;
    }

    void Socket::reset() noexcept {
        if (m_fd >= 0) ::close(m_fd);
        m_fd = -1;
    }

    bool Socket::setNonBlocking() const noexcept {
        const int flags = ::fcntl(m_fd, F_GETFL, 0);
        return flags >= 0 && ::fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) == 0;
    }
} // namespace Common
