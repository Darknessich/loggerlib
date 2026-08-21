#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

namespace Collector {
    class IReceiver {
    public:
        IReceiver() = default;
        virtual ~IReceiver() = default;

        IReceiver(const IReceiver&) = delete;
        IReceiver& operator=(const IReceiver&) = delete;
        IReceiver(IReceiver&&) = delete;
        IReceiver& operator=(IReceiver&&) = delete;

        [[nodiscard]] virtual bool receive(
            std::vector<std::string>& lines, std::chrono::milliseconds timeout, std::error_code& ec
        ) = 0;

        [[nodiscard]] virtual std::size_t dropped() const noexcept = 0;
        [[nodiscard]] virtual std::uint16_t port() const noexcept = 0;
        [[nodiscard]] virtual const std::string& address() const noexcept = 0;
    };
} // namespace Collector
