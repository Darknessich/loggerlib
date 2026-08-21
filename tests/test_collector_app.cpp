#include <framework/TestFramework.hpp>

#include <core/CollectorApp.hpp>

#include <logger/LogLevel.hpp>
#include <logger/LogRecord.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <deque>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

using Collector::CollectorApp;
using Collector::EExitCode;
using Collector::IReceiver;
using Collector::SRun;
using Logger::ELogLevel;

namespace {
    std::string recordOf(ELogLevel level, const std::string& message) {
        return Logger::formatRecord(std::chrono::system_clock::now(), level, message);
    }

    class ScriptedReceiver final : public IReceiver {
    public:
        explicit ScriptedReceiver(std::atomic<bool>& stopping) noexcept : m_stopping{&stopping} {}

        void give(std::vector<std::string> batch) { m_batches.push_back(std::move(batch)); }

        void failAfterTheScript(std::error_code ec) noexcept { m_failure = ec; }

        void dropOneLine() noexcept { ++m_dropped; }

        void idleRounds(int rounds) noexcept { m_idle = rounds; }

        bool receive(
            std::vector<std::string>& lines, std::chrono::milliseconds timeout, std::error_code& ec
        ) override {
            if (!m_batches.empty()) {
                const auto batch = std::move(m_batches.front());
                m_batches.pop_front();
                lines.insert(lines.end(), batch.begin(), batch.end());
                return true;
            }

            if (m_idle > 0) {
                --m_idle;
                std::this_thread::sleep_for(timeout);
                return true;
            }

            m_stopping->store(true);
            if (!m_failure) return true;

            ec = m_failure;
            return false;
        }

        [[nodiscard]] std::size_t dropped() const noexcept override { return m_dropped; }
        [[nodiscard]] std::uint16_t port() const noexcept override { return 5555; }
        [[nodiscard]] const std::string& address() const noexcept override { return m_address; }

    private:
        std::atomic<bool>* m_stopping;
        std::deque<std::vector<std::string>> m_batches;
        std::error_code m_failure;
        std::string m_address{"127.0.0.1"};
        std::size_t m_dropped{0};
        int m_idle{0};
    };

    struct SSession {
        EExitCode code{EExitCode::Success};
        std::string out;
        std::string err;
    };

    SSession run(ScriptedReceiver& receiver, std::atomic<bool>& stopping, const SRun& settings) {
        std::ostringstream out;
        std::ostringstream err;

        CollectorApp application{receiver, settings, out, err, stopping};
        SSession session;
        session.code = application.run();
        session.out = out.str();
        session.err = err.str();
        return session;
    }

    SRun settingsOf(std::size_t every, std::chrono::milliseconds timeout) {
        SRun settings;
        settings.every = every;
        settings.timeout = timeout;
        return settings;
    }

    std::size_t countOf(const std::string& text, const std::string& part) {
        std::size_t found = 0;
        for (auto at = text.find(part); at != std::string::npos; at = text.find(part, at + 1)) {
            ++found;
        }
        return found;
    }
} // namespace

TEST(collector_app, prints_every_record_it_takes) {
    std::atomic<bool> stopping{false};
    ScriptedReceiver receiver{stopping};

    const std::string first = recordOf(ELogLevel::Info, "hello");
    const std::string second = recordOf(ELogLevel::Warn, "disk almost full");
    receiver.give({first, second});

    const auto session = run(receiver, stopping, settingsOf(100, std::chrono::seconds{10}));

    CHECK_EQ(session.code, EExitCode::Success);
    CHECK(session.out.find(first) != std::string::npos);
    CHECK(session.out.find(second) != std::string::npos);
}

TEST(collector_app, reports_after_the_count_is_reached) {
    std::atomic<bool> stopping{false};
    ScriptedReceiver receiver{stopping};

    receiver.give({recordOf(ELogLevel::Info, "one"), recordOf(ELogLevel::Info, "two")});
    receiver.give({recordOf(ELogLevel::Info, "three")});

    const auto session = run(receiver, stopping, settingsOf(2, std::chrono::seconds{10}));

    CHECK(session.out.find("after 2 messages") != std::string::npos);
    CHECK(session.out.find("total 2") != std::string::npos);

    CHECK(session.out.find("(final)") != std::string::npos);
    CHECK(session.out.find("total 3") != std::string::npos);
}

TEST(collector_app, reports_on_the_timeout_when_something_arrived) {
    std::atomic<bool> stopping{false};
    ScriptedReceiver receiver{stopping};

    receiver.give({recordOf(ELogLevel::Info, "alone")});
    receiver.idleRounds(1);

    const auto session = run(receiver, stopping, settingsOf(100, std::chrono::milliseconds{10}));

    CHECK(session.out.find("on timeout") != std::string::npos);
}

TEST(collector_app, keeps_quiet_on_the_timeout_when_nothing_arrived) {
    std::atomic<bool> stopping{false};
    ScriptedReceiver receiver{stopping};

    receiver.idleRounds(2);

    const auto session = run(receiver, stopping, settingsOf(100, std::chrono::milliseconds{10}));

    CHECK(session.out.find("on timeout") == std::string::npos);
    CHECK_EQ(countOf(session.out, "--- statistics"), std::size_t{1}); // the final one
}

TEST(collector_app, reports_the_same_statistics_once_per_timeout) {
    std::atomic<bool> stopping{false};
    ScriptedReceiver receiver{stopping};

    receiver.give({recordOf(ELogLevel::Info, "alone")});
    receiver.idleRounds(3);

    const auto session = run(receiver, stopping, settingsOf(100, std::chrono::milliseconds{10}));

    CHECK_EQ(countOf(session.out, "on timeout"), std::size_t{1});
}

TEST(collector_app, counts_a_line_that_is_not_a_record) {
    std::atomic<bool> stopping{false};
    ScriptedReceiver receiver{stopping};

    receiver.give({recordOf(ELogLevel::Info, "good"), "rubbish"});

    const auto session = run(receiver, stopping, settingsOf(100, std::chrono::seconds{10}));

    CHECK(session.err.find("not a record: rubbish") != std::string::npos);
    CHECK(session.out.find("rubbish") == std::string::npos);
    CHECK(session.out.find("malformed 1") != std::string::npos);
    CHECK(session.out.find("total 1") != std::string::npos);
}

TEST(collector_app, an_empty_line_is_neither_a_record_nor_a_mistake) {
    std::atomic<bool> stopping{false};
    ScriptedReceiver receiver{stopping};

    receiver.give({"", recordOf(ELogLevel::Info, "kept"), ""});

    const auto session = run(receiver, stopping, settingsOf(100, std::chrono::seconds{10}));

    CHECK(session.err.empty());
    CHECK(session.out.find("malformed 0") != std::string::npos);
    CHECK(session.out.find("total 1") != std::string::npos);
}

TEST(collector_app, escapes_what_it_repeats_of_a_broken_line) {
    std::atomic<bool> stopping{false};
    ScriptedReceiver receiver{stopping};

    receiver.give({"\x1b[2J clear the screen"});

    const auto session = run(receiver, stopping, settingsOf(100, std::chrono::seconds{10}));

    CHECK(session.err.find("\\x1B") != std::string::npos);
    CHECK(session.err.find('\x1b') == std::string::npos);
}

TEST(collector_app, counts_what_the_receiver_had_to_drop) {
    std::atomic<bool> stopping{false};
    ScriptedReceiver receiver{stopping};

    receiver.give({recordOf(ELogLevel::Info, "kept")});
    receiver.dropOneLine();
    receiver.give({recordOf(ELogLevel::Info, "kept too")});

    const auto session = run(receiver, stopping, settingsOf(100, std::chrono::seconds{10}));

    CHECK(session.out.find("malformed 1") != std::string::npos);
    CHECK(session.out.find("total 2") != std::string::npos);
}

TEST(collector_app, names_a_receiver_that_broke) {
    std::atomic<bool> stopping{false};
    ScriptedReceiver receiver{stopping};

    receiver.give({recordOf(ELogLevel::Info, "before the end")});
    receiver.failAfterTheScript(std::make_error_code(std::errc::bad_file_descriptor));

    const auto session = run(receiver, stopping, settingsOf(100, std::chrono::seconds{10}));

    CHECK_EQ(session.code, EExitCode::ReceiveFailed);
    CHECK(
        session.err.find(std::make_error_code(std::errc::bad_file_descriptor).message()) !=
        std::string::npos
    );
    CHECK(session.out.find("stopped by a failure") != std::string::npos);
}
