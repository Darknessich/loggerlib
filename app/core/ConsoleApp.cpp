#include "ConsoleApp.hpp"
#include "EventQueue.hpp"
#include "InputParser.hpp"
#include "LogWorker.hpp"
#include "Options.hpp"

#include <logger/LogLevel.hpp>

#include <cstddef>
#include <istream>
#include <ostream>
#include <string>
#include <utility>

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

            SUserInput input = parseUserInput(line, level);
            switch (input.kind) {
                case EInputKind::Message:
                    running =
                        queue.push({EEventKind::Write, input.level, std::move(input.message)});
                    break;
                case EInputKind::SetLevel:
                    level = input.level;
                    running = queue.push({EEventKind::SetLevel, input.level, {}});
                    m_out << "level: " << Logger::level2string(input.level) << '\n';
                    break;
                case EInputKind::Help:
                    printHelp(m_out);
                    break;
                case EInputKind::Quit:
                    running = false;
                    break;
                case EInputKind::Empty:
                    break;
                case EInputKind::Error:
                    m_err << input.error << '\n';
                    break;
            }

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
