#include "Address.hpp"

#include <cstring>

namespace Logger {
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

    std::vector<SEndpoint>
    resolve(const std::string& host, std::uint16_t port, int socktype, std::error_code& ec) {
        ec.clear();

        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = socktype;
        hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV;

        const std::string service = std::to_string(port);

        AddressList list;
        const int result = ::getaddrinfo(host.c_str(), service.c_str(), &hints, list.out());
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
} // namespace Logger
