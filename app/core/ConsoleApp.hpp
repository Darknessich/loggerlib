#pragma once

#include <logger/ILogger.hpp>

#include <iosfwd>

namespace App {
    enum class EExitCode {
        Success        = 0,
        Usage          = 1,
        LogUnavailable = 2,
        WriteFailed    = 3
    };

    class ConsoleApp {
    public:
        ConsoleApp(Logger::ILogger& logger, std::istream& in,
                    std::ostream& out, std::ostream& err, bool showPrompt);

        ConsoleApp(const ConsoleApp&) = delete;
        ConsoleApp& operator=(const ConsoleApp&) = delete;
        ConsoleApp(ConsoleApp&&) = delete;
        ConsoleApp& operator=(ConsoleApp&&) = delete;

        EExitCode run();

    private:
        Logger::ILogger& m_logger;
        std::istream& m_in;
        std::ostream& m_out;
        std::ostream& m_err;
        bool m_showPrompt{true};
    };
} // namespace App
