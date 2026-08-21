#include <common/net/Address.hpp>

#include <cstring>

#include <arpa/inet.h>
#include <netinet/in.h>

namespace Common {
    namespace {
        class AddressCategory final : public std::error_category {
        public:
            [[nodiscard]] const char* name() const noexcept override { return "getaddrinfo"; }

            [[nodiscard]] std::string message(int value) const override {
                return ::gai_strerror(value);
            }
        };

        const std::error_category& addressCategory() {
            static const AddressCategory category;
            return category;
        }

        class AddressList {
        public:
            ~AddressList() {
                if (m_head != nullptr) ::freeaddrinfo(m_head);
            }

            AddressList() = default;
            AddressList(const AddressList&) = delete;
            AddressList& operator=(const AddressList&) = delete;
            AddressList(AddressList&&) = delete;
            AddressList& operator=(AddressList&&) = delete;

            addrinfo** out() noexcept { return &m_head; }
            [[nodiscard]] const addrinfo* head() const noexcept { return m_head; }

        private:
            addrinfo* m_head = nullptr;
        };
    } // namespace

    std::vector<SEndpoint> resolve(
        const std::string& host,
        std::uint16_t port,
        int socktype,
        std::error_code& ec,
        int extraFlags
    ) {
        ec.clear();

        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = socktype;
        hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV | extraFlags;

        const std::string service = std::to_string(port);

        AddressList list;
        const char* const name = host.empty() ? nullptr : host.c_str();
        const int result = ::getaddrinfo(name, service.c_str(), &hints, list.out());
        if (result != 0) {
            ec = std::error_code{result, addressCategory()};
            return {};
        }

        std::vector<SEndpoint> endpoints;
        for (const addrinfo* entry = list.head(); entry != nullptr; entry = entry->ai_next) {
            SEndpoint endpoint;
            endpoint.family = entry->ai_family;
            endpoint.socktype = entry->ai_socktype;
            endpoint.protocol = entry->ai_protocol;
            endpoint.length = entry->ai_addrlen;
            std::memcpy(&endpoint.address, entry->ai_addr, entry->ai_addrlen);
            endpoints.push_back(endpoint);
        }

        if (endpoints.empty()) ec = std::error_code{EAI_NONAME, addressCategory()};
        return endpoints;
    }

    const sockaddr* asSockaddr(const SEndpoint& endpoint) noexcept {
        return reinterpret_cast<const sockaddr*>(&endpoint.address);
    }

    std::optional<std::uint16_t> localPort(int fd) noexcept {
        sockaddr_storage address{};
        auto length = static_cast<socklen_t>(sizeof(address));
        if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &length) != 0)
            return std::nullopt;

        if (address.ss_family == AF_INET)
            return ntohs(reinterpret_cast<const sockaddr_in*>(&address)->sin_port);
        if (address.ss_family == AF_INET6)
            return ntohs(reinterpret_cast<const sockaddr_in6*>(&address)->sin6_port);

        return std::nullopt;
    }

    std::optional<std::string> localAddress(int fd) {
        sockaddr_storage address{};
        auto length = static_cast<socklen_t>(sizeof(address));
        if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &length) != 0)
            return std::nullopt;

        char text[INET6_ADDRSTRLEN] = {};
        const void* raw = nullptr;
        if (address.ss_family == AF_INET)
            raw = &reinterpret_cast<const sockaddr_in*>(&address)->sin_addr;
        else if (address.ss_family == AF_INET6)
            raw = &reinterpret_cast<const sockaddr_in6*>(&address)->sin6_addr;
        else
            return std::nullopt;

        if (::inet_ntop(address.ss_family, raw, text, sizeof(text)) == nullptr) return std::nullopt;

        return std::string{text};
    }
} // namespace Common
