#pragma once

#include <cerrno>
#include <system_error>

namespace Common {
    /// @brief Turns the errno left by a failed system call into an error code.
    /// @return the code: std::errc::io_error when errno was not set
    inline std::error_code errnoError() noexcept {
        return errno != 0 ? std::error_code{errno, std::generic_category()}
                          : std::make_error_code(std::errc::io_error);
    }
} // namespace Common
