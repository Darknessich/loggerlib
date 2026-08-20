#include "ConsoleApp.hpp"
#include "EventQueue.hpp"
#include "InputParser.hpp"
#include "LogWorker.hpp"
#include "Options.hpp"
#include "Overloaded.hpp"

#include <logger/LogLevel.hpp>

#include <cstddef>
#include <istream>
#include <ostream>
#include <string>
#include <utility>
#include <variant>

namespace App {
    namespace {
        void printHelp(std::ostream& out) {
            out << "Type a message and press Enter. An optional first word sets its level.\n"
                   "Levels: ";
            printLevels(out);
            out << "\n"
                   "Commands:\n"
                   "  /level <name>  change the level of the running logger\n"
                   "  /help          show this text\n"
                   "  /quit          stop the application, same as Ctrl-D\n"
                   "  //<text>       log a message that starts with a slash\n";
        }
    } // namespace

    ConsoleApp::ConsoleApp(
        Logger::ILogger& logger,
        std::istream& in,
        std::ostream& out,
        std::ostream& err,
        bool showPrompt
    )
        : m_logger{logger}, m_in{in}, m_out{out}, m_err{err}, m_showPrompt{showPrompt} {}

    EExitCode ConsoleApp::run() {
        EventQueue queue;
        LogWorker worker{m_logger, queue};
        worker.start();

        bool running = true;
        Logger::ELogLevel level = m_logger.level();
        std::string reportedReason;
        std::string line;

        while (running) {
            if (m_showPrompt) m_out << "> " << std::flush;
            if (!std::getline(m_in, line)) break;
            if (!line.empty() && line.back() == '\r') line.pop_back();

            TUserInput input = parseUserInput(line, level);
            std::visit(
                SOverloaded{
                    [&](SMessage& message) {
                        running = queue.push(SWrite{message.level, std::move(message.text)});
                    },
                    [&](const SLevelCommand& command) {
                        level = command.level;
                        running = queue.push(SSetLevel{command.level});
                        m_out << "level: " << Logger::level2string(command.level) << '\n';
                    },
                    [&](const SHelpCommand&) { printHelp(m_out); },
                    [&](const SQuitCommand&) { running = false; },
                    [](const SEmptyLine&) {},
                    [&](const SError& error) { m_err << error.text << '\n'; }
                },
                input
            );

            std::string reason = worker.lastError();
            if (!reason.empty() && reason != reportedReason) {
                m_err << "log write failed: " << reason << '\n';
                reportedReason = std::move(reason);
            }
        }

        if (m_showPrompt && running) m_out << '\n';

        worker.stop();

        const std::size_t failed = worker.failed();
        m_err << "processed " << worker.processed() << " message(s), " << failed << " failed";
        if (failed > 0) {
            const std::string reason = worker.lastError();
            if (!reason.empty()) m_err << ": " << reason;
        }
        m_err << '\n';
        return failed > 0 ? EExitCode::WriteFailed : EExitCode::Success;
    }
} // namespace App
