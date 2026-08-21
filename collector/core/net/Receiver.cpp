#include "Receiver.hpp"

#include "TcpReceiver.hpp"
#include "UdpReceiver.hpp"

namespace Collector {
    std::unique_ptr<IReceiver> openReceiver(
        const std::string& host,
        std::uint16_t port,
        Logger::ESocketProtocol protocol,
        std::error_code& ec
    ) {
        ec.clear();

        switch (protocol) {
            case Logger::ESocketProtocol::Tcp:
                return openTcpReceiver(host, port, ec);
            case Logger::ESocketProtocol::Udp:
                return openUdpReceiver(host, port, ec);
            case Logger::ESocketProtocol::Count:
            default:
                ec = std::make_error_code(std::errc::invalid_argument);
                return nullptr;
        }
    }
} // namespace Collector
