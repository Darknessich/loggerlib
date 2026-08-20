#include <framework/TestFramework.hpp>

#include <ISink.hpp>
#include <MultiSink.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {
    struct SSinkState {
        std::vector<std::string> lines;
        std::error_code failure;
    };

    class SinkProxy final : public Logger::ISink {
    public:
        explicit SinkProxy(SSinkState& state) noexcept : m_state{&state} {}

        bool writeLine(std::string_view line, std::error_code& ec) override {
            m_state->lines.emplace_back(line);
            if (!m_state->failure) return true;

            ec = m_state->failure;
            return false;
        }

    private:
        SSinkState* m_state;
    };

    Logger::MultiSink combine(const std::vector<SSinkState*>& states) {
        std::vector<std::unique_ptr<Logger::ISink>> sinks;
        sinks.reserve(states.size());
        for (SSinkState* state : states) {
            sinks.push_back(std::make_unique<SinkProxy>(*state));
        }

        return Logger::MultiSink{std::move(sinks)};
    }
} // namespace

TEST(multi_sink, writes_the_same_line_to_every_sink) {
    SSinkState first;
    SSinkState second;
    auto sink = combine({&first, &second});

    std::error_code ec;
    CHECK(sink.writeLine("line", ec));
    CHECK(!ec);

    REQUIRE_EQ(first.lines.size(), std::size_t{1});
    REQUIRE_EQ(second.lines.size(), std::size_t{1});
    CHECK_EQ(first.lines.front(), "line");
    CHECK_EQ(second.lines.front(), "line");
}

TEST(multi_sink, a_failing_sink_does_not_stop_the_others) {
    SSinkState first;
    SSinkState failing;
    SSinkState last;
    failing.failure = std::make_error_code(std::errc::broken_pipe);

    auto sink = combine({&first, &failing, &last});

    std::error_code ec;
    CHECK(!sink.writeLine("line", ec));
    CHECK(ec == std::errc::broken_pipe);

    CHECK_EQ(first.lines.size(), std::size_t{1});
    CHECK_EQ(last.lines.size(), std::size_t{1});
}

TEST(multi_sink, reports_the_first_failure) {
    SSinkState first;
    SSinkState second;
    first.failure = std::make_error_code(std::errc::no_space_on_device);
    second.failure = std::make_error_code(std::errc::broken_pipe);

    auto sink = combine({&first, &second});

    std::error_code ec;
    CHECK(!sink.writeLine("line", ec));
    CHECK(ec == std::errc::no_space_on_device);
}

TEST(multi_sink, a_recovered_sink_makes_the_write_succeed_again) {
    SSinkState first;
    SSinkState second;
    second.failure = std::make_error_code(std::errc::broken_pipe);

    auto sink = combine({&first, &second});

    std::error_code ec;
    CHECK(!sink.writeLine("lost", ec));

    second.failure.clear();
    std::error_code again;
    CHECK(sink.writeLine("kept", again));
    CHECK(!again);

    CHECK_EQ(first.lines.size(), std::size_t{2});
    CHECK_EQ(second.lines.size(), std::size_t{2});
}
