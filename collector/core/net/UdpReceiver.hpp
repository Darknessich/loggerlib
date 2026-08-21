#pragma once

#include "../IReceiver.hpp"
#include "Listener.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Collector {
    class UdpReceiver final : public IReceiver {
    public:
        explicit UdpReceiver(SListener listener) noexcept;

        bool receive(
            std::vector<std::string>& lines, std::chrono::milliseconds timeout, std::error_code& ec
        ) override;

        [[nodiscard]] std::size_t dropped() const noexcept override;
        [[nodiscard]] std::uint16_t port() const noexcept override;
        [[nodiscard]] const std::string& address() const noexcept override;

    private:
        SListener m_listener;
    };

    std::unique_ptr<IReceiver>
    openUdpReceiver(const std::string& host, std::uint16_t port, std::error_code& ec);
} // namespace Collector
