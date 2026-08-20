#include <logger/SocketProtocol.hpp>

#include "Text.hpp"

#include <array>
#include <cstddef>

namespace Logger {
    namespace {
        struct SProtocolName {
            ESocketProtocol protocol;
            std::string_view name;
        };

        inline constexpr std::size_t kProtocolCount =
            static_cast<std::size_t>(ESocketProtocol::Count);
        inline constexpr std::array<SProtocolName, kProtocolCount> kProtocolNames{
            {{ESocketProtocol::Tcp, "tcp"}, {ESocketProtocol::Udp, "udp"}}
        };

        constexpr bool isTableOrdered() noexcept {
            for (std::size_t i = 0; i < kProtocolCount; ++i) {
                if (static_cast<std::size_t>(kProtocolNames[i].protocol) != i) {
                    return false;
                }
            }
            return true;
        }

        static_assert(isTableOrdered(), "kProtocolNames must be ordered by enum value");
    } // namespace

    std::string_view protocol2string(ESocketProtocol protocol) noexcept {
        const std::size_t index = static_cast<std::size_t>(protocol);
        return isValidProtocol(protocol) ? kProtocolNames[index].name : std::string_view{"unknown"};
    }

    std::optional<ESocketProtocol> string2protocol(std::string_view str) noexcept {
        for (const auto& entry : kProtocolNames) {
            if (equalsIgnoreCase(entry.name, str)) {
                return entry.protocol;
            }
        }
        return std::nullopt;
    }

    bool isValidProtocol(ESocketProtocol protocol) noexcept {
        return protocol < ESocketProtocol::Count;
    }
} // namespace Logger
