#pragma once

#include <fcntl.h>
#include <unistd.h>

namespace Logger {
    // Owns a descriptor, move-only
    class Socket {
    public:
        Socket() noexcept = default;
        explicit Socket(int fd) noexcept;
        ~Socket();

        Socket(Socket&& other) noexcept;
        Socket& operator=(Socket&& other) noexcept;

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        [[nodiscard]] bool valid() const noexcept;
        [[nodiscard]] int get() const noexcept;

        int release() noexcept;
        void reset() noexcept;

        // SOCK_NONBLOCK exists only on Linux
        [[nodiscard]] bool setNonBlocking() const noexcept;

    private:
        int m_fd{-1};
    };
} // namespace Logger
