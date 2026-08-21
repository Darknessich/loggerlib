#pragma once

#include <fcntl.h>
#include <unistd.h>

namespace Common {
    /// @brief Owns a descriptor, move-only.
    class Socket {
    public:
        /// @brief Owns nothing.
        Socket() noexcept = default;

        /// @brief Takes over @p fd.
        /// @param fd descriptor to own, a negative value means none
        explicit Socket(int fd) noexcept;

        /// @brief Closes the descriptor.
        ~Socket();

        /// @brief Takes the descriptor of @p other, leaving it empty.
        /// @param other socket to take from
        Socket(Socket&& other) noexcept;

        /// @brief Closes the own descriptor and takes the one of @p other.
        /// @param other socket to take from
        /// @return this socket
        Socket& operator=(Socket&& other) noexcept;

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        /// @brief Returns true when a descriptor is owned.
        [[nodiscard]] bool valid() const noexcept;

        /// @brief Returns the descriptor without giving up ownership.
        [[nodiscard]] int get() const noexcept;

        /// @brief Gives up ownership.
        /// @return the descriptor, now the caller's to close
        int release() noexcept;

        /// @brief Closes the descriptor and owns nothing again.
        void reset() noexcept;

        /// @brief Switches the descriptor to non-blocking mode.
        /// @return false when the descriptor did not take the mode
        /// @note fcntl and not SOCK_NONBLOCK, which exists only on Linux
        [[nodiscard]] bool setNonBlocking() const noexcept;

    private:
        int m_fd{-1};
    };
} // namespace Common
