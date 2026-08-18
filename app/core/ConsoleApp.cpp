#include "ConsoleApp.hpp"
#include "InputParser.hpp"
#include "LogWorker.hpp"
#include "MessageQueue.hpp"
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
        MessageQueue queue;
        LogWorker worker{m_logger, queue};
        worker.start();

        bool running = true;
        bool warned = false;
        std::string line;

        while (running) {
            if (m_showPrompt) m_out << "> " << std::flush;
            if (!std::getline(m_in, line)) break;

            SUserInput input = parseUserInput(line, m_logger.level());
            switch (input.kind) {
                case EInputKind::Message:
                    if (m_logger.isEnabled(input.level))
                        running = queue.push({input.level, std::move(input.message)});
                    break;
                case EInputKind::SetLevel:
                    m_logger.setLevel(input.level);
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

            if (!warned && worker.failed() > 0) {
                m_err << "log write failed, further failures are only counted\n";
                warned = true;
            }
        }

        if (m_showPrompt && running) m_out << '\n';

        worker.stop();

        const std::size_t failed = worker.failed();
        m_err << "processed " << worker.processed() << " message(s), " << failed << " failed\n";
        return failed > 0 ? EExitCode::WriteFailed : EExitCode::Success;
    }
} // namespace App
