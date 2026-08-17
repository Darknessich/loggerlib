#include <core/ConsoleApp.hpp>
#include <core/Options.hpp>

#include <logger/LoggerFactory.hpp>

#include <iostream>
#include <system_error>

#include <unistd.h>

int main(int argc, char** argv) {
    const char* const program = argc > 0 ? argv[0] : "logger_app";

    const App::SOptions options = App::parseOptions(argc, argv);
    switch (options.kind) {
        case App::EOptionsKind::Help:
            App::printUsage(program, std::cout);
            return static_cast<int>(App::EExitCode::Success);
        case App::EOptionsKind::Error:
            std::cerr << options.error << '\n';
            App::printUsage(program, std::cerr);
            return static_cast<int>(App::EExitCode::Usage);
        case App::EOptionsKind::Run:
            break;
    }

    std::error_code ec;
    const auto logger = Logger::createFileLogger(options.path, options.level, ec);
    if (!logger) {
        std::cerr << "cannot open '" << options.path << "': " << ec.message() << '\n';
        return static_cast<int>(App::EExitCode::LogUnavailable);
    }

    App::ConsoleApp application{
        *logger, std::cin, std::cout,
        std::cerr, ::isatty(STDIN_FILENO) != 0
    };
    return static_cast<int>(application.run());
}
