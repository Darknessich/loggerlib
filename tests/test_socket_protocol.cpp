#include <framework/TestFramework.hpp>

#include <logger/SocketProtocol.hpp>

#include <cstddef>

using Logger::ESocketProtocol;

TEST(socket_protocol, round_trips_every_protocol) {
    for (std::size_t index = 0; index < static_cast<std::size_t>(ESocketProtocol::Count); ++index) {
        const auto protocol = static_cast<ESocketProtocol>(index);
        const auto name = Logger::protocol2string(protocol);

        CHECK(!name.empty());
        CHECK_EQ(Logger::string2protocol(name), protocol);
    }
}

TEST(socket_protocol, parsing_ignores_case) {
    CHECK_EQ(Logger::string2protocol("TCP"), ESocketProtocol::Tcp);
    CHECK_EQ(Logger::string2protocol("Udp"), ESocketProtocol::Udp);
}

TEST(socket_protocol, rejects_an_unknown_name) {
    for (const auto* const name : {"", " tcp", "tcp ", "sctp", "unknown"}) {
        CHECK(!Logger::string2protocol(name).has_value());
    }
}

TEST(socket_protocol, names_an_out_of_range_value_unknown) {
    for (const auto protocol :
         {ESocketProtocol::Count,
          static_cast<ESocketProtocol>(42),
          static_cast<ESocketProtocol>(255)}) {
        CHECK(!Logger::isValidProtocol(protocol));
        CHECK_EQ(Logger::protocol2string(protocol), "unknown");
    }
}
