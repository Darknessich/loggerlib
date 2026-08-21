#include "CollectorApp.hpp"

#include <logger/LogRecord.hpp>

#include <algorithm>
#include <ostream>
#include <system_error>
#include <utility>
#include <vector>

namespace Collector {
    namespace {
        constexpr std::size_t kPreview = 80;

        std::string preview(const std::string& line) {
            const std::string cut = Logger::escapeMessage(line.substr(0, kPreview));
            return line.size() > kPreview ? cut + "..." : cut;
        }
    } // namespace

    CollectorApp::CollectorApp(
        IReceiver& receiver,
        SRun settings,
        std::ostream& out,
        std::ostream& err,
        const std::atomic<bool>& stopping
    )
        : m_receiver{receiver}, m_settings{std::move(settings)}, m_out{out}, m_err{err},
          m_stopping{stopping} {}

    EExitCode CollectorApp::run() {
        auto deadline = TClock::now() + m_settings.timeout;

        std::vector<std::string> lines;
        std::error_code ec;

        while (!m_stopping.load()) {
            const auto left =
                std::chrono::ceil<std::chrono::milliseconds>(deadline - TClock::now());
            const auto wait =
                std::clamp(left, std::chrono::milliseconds::zero(), m_settings.timeout);

            lines.clear();
            if (!m_receiver.receive(lines, wait, ec)) {
                m_err << "cannot receive: " << ec.message() << '\n';
                report("stopped by a failure");
                return EExitCode::ReceiveFailed;
            }

            for (const std::string& line : lines) {
                consume(line);
            }
            countDropped();

            if (m_sinceReport >= m_settings.every) {
                report("after " + std::to_string(m_sinceReport) + " messages");
                deadline = TClock::now() + m_settings.timeout;
            } else if (TClock::now() >= deadline) {
                if (m_sinceReport > 0) report("on timeout");
                deadline = TClock::now() + m_settings.timeout;
            }
        }

        report("final");
        return EExitCode::Success;
    }

    void CollectorApp::consume(const std::string& line) {
        if (line.empty()) return;

        ++m_sinceReport;

        const auto record = Logger::parseRecord(line);
        if (!record) {
            m_statistics.addMalformed();
            m_err << "not a record: " << preview(line) << '\n';
            return;
        }

        m_out << line << '\n' << std::flush;
        m_statistics.add(record->level, record->message.size(), TClock::now());
    }

    void CollectorApp::countDropped() {
        for (const std::size_t dropped = m_receiver.dropped(); m_dropped < dropped; ++m_dropped) {
            m_statistics.addMalformed();
            ++m_sinceReport;
        }
    }

    void CollectorApp::report(const std::string& reason) {
        m_out << formatReport(m_statistics.snapshot(TClock::now()), reason) << std::flush;
        m_sinceReport = 0;
    }
} // namespace Collector
