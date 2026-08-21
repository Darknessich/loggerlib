#include <core/CollectorApp.hpp>
#include <core/Options.hpp>
#include <core/net/Receiver.hpp>

#include <common/Endpoint.hpp>
#include <common/Overloaded.hpp>

#include <logger/SocketProtocol.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <exception>
#include <iostream>
#include <system_error>
#include <variant>

namespace {
    std::atomic<bool>& stopFlag() noexcept {
        static std::atomic<bool> flag{false};
        static_assert(std::atomic<bool>::is_always_lock_free);
        return flag;
    }

    extern "C" void requestStop(int /*signal*/) {
        stopFlag().store(true);
    }

    Collector::EExitCode runCollector(const Collector::SRun& options) {
        std::error_code ec;
        const auto receiver =
            Collector::openReceiver(options.host, options.port, options.protocol, ec);
        if (!receiver) {
            std::cerr << "cannot listen: " << ec.message() << '\n';
            return Collector::EExitCode::ListenFailed;
        }

        std::cout << "listening on " << Common::joinHostPort(receiver->address(), receiver->port())
                  << ' ' << Logger::protocol2string(options.protocol) << ", reporting every "
                  << options.every << " messages or "
                  << std::chrono::duration_cast<std::chrono::seconds>(options.timeout).count()
                  << " s\n"
                  << std::flush;

        (void)std::signal(SIGINT, &requestStop);
        (void)std::signal(SIGTERM, &requestStop);

        Collector::CollectorApp application{*receiver, options, std::cout, std::cerr, stopFlag()};
        return application.run();
    }
} // namespace

int main(int argc, char** argv) {
    try {
        const char* const program = argc > 0 ? argv[0] : "logger_collector";

        const Collector::EExitCode code = std::visit(
            Common::SOverloaded{
                [](const Collector::SRun& options) { return runCollector(options); },
                [program](const Collector::SShowHelp&) {
                    Collector::printUsage(program, std::cout);
                    return Collector::EExitCode::Success;
                },
                [program](const Collector::SUsageError& error) {
                    std::cerr << error.text << '\n';
                    Collector::printUsage(program, std::cerr);
                    return Collector::EExitCode::Usage;
                }
            },
            Collector::parseOptions(argc, argv)
        );

        return static_cast<int>(code);
    } catch (const std::exception& error) {
        std::cerr << "internal error: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "internal error: unknown exception\n";
    }
    return static_cast<int>(Collector::EExitCode::InternalError);
}
