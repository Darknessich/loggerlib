#pragma once

#include "../IReceiver.hpp"

#include <logger/SocketProtocol.hpp>

#include <memory>
#include <string>

namespace Collector {
    std::unique_ptr<IReceiver> openReceiver(
        const std::string& host,
        std::uint16_t port,
        Logger::ESocketProtocol protocol,
        std::error_code& ec
    );
} // namespace Collector
