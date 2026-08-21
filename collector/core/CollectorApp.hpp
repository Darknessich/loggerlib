#pragma once

#include "IReceiver.hpp"
#include "Options.hpp"
#include "Statistics.hpp"

#include <atomic>
#include <cstddef>
#include <iosfwd>
#include <string>

namespace Collector {
    enum class EExitCode {
        Success = 0,
        Usage = 1,
        ListenFailed = 2,
        ReceiveFailed = 3,
        InternalError = 4
    };

    class CollectorApp {
    public:
        CollectorApp(
            IReceiver& receiver,
            SRun settings,
            std::ostream& out,
            std::ostream& err,
            const std::atomic<bool>& stopping
        );

        ~CollectorApp() = default;

        CollectorApp(const CollectorApp&) = delete;
        CollectorApp& operator=(const CollectorApp&) = delete;
        CollectorApp(CollectorApp&&) = delete;
        CollectorApp& operator=(CollectorApp&&) = delete;

        EExitCode run();

    private:
        void consume(const std::string& line);
        void report(const std::string& reason);
        void countDropped();

        IReceiver& m_receiver;
        SRun m_settings;
        std::ostream& m_out;
        std::ostream& m_err;
        const std::atomic<bool>& m_stopping;

        Statistics m_statistics;
        std::size_t m_sinceReport{0};
        std::size_t m_dropped{0};
    };
} // namespace Collector
