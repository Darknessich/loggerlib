#include <logger/LoggerFactory.hpp>

#include "FileSink.hpp"
#include "MultiSink.hpp"
#include "SinkLogger.hpp"
#include "net/TcpSink.hpp"
#include "net/UdpSink.hpp"
#include <common/Overloaded.hpp>

#include <utility>

namespace Logger {
    namespace {
        std::unique_ptr<ISink> openSink(const TTarget& target, std::error_code& ec) {
            return std::visit(
                Common::SOverloaded{
                    [&](const SFileTarget& file) { return openFileSink(file.path, ec); },
                    [&](const SSocketTarget& socket) -> std::unique_ptr<ISink> {
                        switch (socket.protocol) {
                            case ESocketProtocol::Tcp:
                                return openTcpSink(socket.host, socket.port, STcpSettings{}, ec);
                            case ESocketProtocol::Udp:
                                return openUdpSink(socket.host, socket.port, ec);
                            case ESocketProtocol::Count:
                                break;
                        }
                        ec = std::make_error_code(std::errc::invalid_argument);
                        return nullptr;
                    }
                },
                target
            );
        }
    } // namespace

    std::unique_ptr<ILogger>
    createLogger(const std::vector<TTarget>& targets, ELogLevel level, std::error_code& ec) {
        ec.clear();

        if (targets.empty() || !isValidLevel(level)) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return nullptr;
        }

        std::vector<std::unique_ptr<ISink>> sinks;
        sinks.reserve(targets.size());
        for (const auto& target : targets) {
            auto sink = openSink(target, ec);
            if (!sink) return nullptr;
            sinks.emplace_back(std::move(sink));
        }

        auto sink = sinks.size() == 1 ? std::move(sinks.front())
                                      : std::make_unique<MultiSink>(std::move(sinks));
        return std::make_unique<SinkLogger>(std::move(sink), level);
    }

    std::unique_ptr<ILogger> createLogger(TTarget target, ELogLevel level, std::error_code& ec) {
        return createLogger(std::vector<TTarget>{std::move(target)}, level, ec);
    }
} // namespace Logger
