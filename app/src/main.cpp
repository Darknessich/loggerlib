#include <common/Overloaded.hpp>
#include <core/ConsoleApp.hpp>
#include <core/Options.hpp>

#include <logger/LoggerFactory.hpp>

#include <exception>
#include <iostream>
#include <system_error>
#include <variant>

#include <unistd.h>

namespace {
    App::EExitCode runApplication(const App::SRun& options) {
        std::error_code ec;
        const auto logger = Logger::createLogger(options.targets, options.level, ec);
        if (!logger) {
            std::cerr << "cannot open the log: " << ec.message() << '\n';
            return App::EExitCode::LogUnavailable;
        }

        App::ConsoleApp application{
            *logger, std::cin, std::cout, std::cerr, ::isatty(STDIN_FILENO) != 0
        };
        return application.run();
    }
} // namespace

int main(int argc, char** argv) {
    try {
        const char* const program = argc > 0 ? argv[0] : "logger_app";

        const App::EExitCode code = std::visit(
            Common::SOverloaded{
                [](const App::SRun& options) { return runApplication(options); },
                [program](const App::SShowHelp&) {
                    App::printUsage(program, std::cout);
                    return App::EExitCode::Success;
                },
                [program](const App::SUsageError& error) {
                    std::cerr << error.text << '\n';
                    App::printUsage(program, std::cerr);
                    return App::EExitCode::Usage;
                }
            },
            App::parseOptions(argc, argv)
        );

        return static_cast<int>(code);
    } catch (const std::exception& error) {
        std::cerr << "internal error: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "internal error: unknown exception\n";
    }
    return static_cast<int>(App::EExitCode::InternalError);
}
