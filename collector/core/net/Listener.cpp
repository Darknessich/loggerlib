#include "Listener.hpp"

#include <common/Errors.hpp>
#include <common/net/Address.hpp>

#include <cerrno>
#include <utility>

#include <sys/socket.h>

namespace Collector {
    std::optional<SListener> bindListener(
        const std::string& host, std::uint16_t port, int socktype, int backlog, std::error_code& ec
    ) {
        const auto endpoints = Common::resolve(host, port, socktype, ec, AI_PASSIVE);
        if (ec) return std::nullopt;

        ec = std::make_error_code(std::errc::address_not_available);
        for (const auto& endpoint : endpoints) {
            errno = 0;
            Common::Socket socket{::socket(endpoint.family, endpoint.socktype, endpoint.protocol)};
            if (!socket.valid() || !socket.setNonBlocking()) {
                ec = Common::errnoError();
                continue;
            }

            const int on = 1;
            (void)::setsockopt(socket.get(), SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

            errno = 0;
            if (::bind(socket.get(), Common::asSockaddr(endpoint), endpoint.length) != 0) {
                ec = Common::errnoError();
                continue;
            }

            if (backlog > 0 && ::listen(socket.get(), backlog) != 0) {
                ec = Common::errnoError();
                continue;
            }

            auto address = Common::localAddress(socket.get());
            const auto bound = Common::localPort(socket.get());
            if (!address || !bound) {
                ec = Common::errnoError();
                continue;
            }

            ec.clear();
            return SListener{std::move(socket), std::move(*address), *bound};
        }

        return std::nullopt;
    }
} // namespace Collector
