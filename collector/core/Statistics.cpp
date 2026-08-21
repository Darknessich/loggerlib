#include "Statistics.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace Collector {
    namespace {
        std::int64_t minuteOf(TClock::time_point now) noexcept {
            return std::chrono::duration_cast<std::chrono::minutes>(now.time_since_epoch()).count();
        }
    } // namespace

    void
    Statistics::add(Logger::ELogLevel level, std::size_t length, TClock::time_point now) noexcept {
        ++m_total;

        const auto index = static_cast<std::size_t>(level);
        if (index < m_byLevel.size()) ++m_byLevel[index];

        m_totalLength += length;
        m_minLength = m_total == 1 ? length : std::min(m_minLength, length);
        m_maxLength = std::max(m_maxLength, length);

        const std::int64_t minute = minuteOf(now);
        SBucket& bucket = m_minutes[static_cast<std::uint64_t>(minute) % kBuckets];
        if (bucket.minute != minute) {
            bucket.minute = minute;
            bucket.count = 0;
        }
        ++bucket.count;
    }

    void Statistics::addMalformed() noexcept {
        ++m_malformed;
    }

    SSnapshot Statistics::snapshot(TClock::time_point now) const noexcept {
        SSnapshot snapshot;
        snapshot.total = m_total;
        snapshot.malformed = m_malformed;
        snapshot.byLevel = m_byLevel;
        snapshot.minLength = m_minLength;
        snapshot.maxLength = m_maxLength;
        snapshot.averageLength =
            m_total == 0 ? 0.0 : static_cast<double>(m_totalLength) / static_cast<double>(m_total);

        const std::int64_t oldest = minuteOf(now) - static_cast<std::int64_t>(kBuckets) + 1;
        for (const SBucket& bucket : m_minutes) {
            if (bucket.minute >= oldest) snapshot.lastHour += bucket.count;
        }

        return snapshot;
    }

    std::string formatReport(const SSnapshot& snapshot, std::string_view reason) {
        std::ostringstream out;
        out << "--- statistics (" << reason << ") ---\n"
            << "total " << snapshot.total << "  last hour " << snapshot.lastHour << "  malformed "
            << snapshot.malformed << '\n'
            << "by level ";

        for (std::size_t i = 0; i < snapshot.byLevel.size(); ++i) {
            out << ' ' << Logger::level2string(static_cast<Logger::ELogLevel>(i)) << ' '
                << snapshot.byLevel[i];
        }

        out << "\nlength   ";
        if (snapshot.total == 0) {
            out << " min -  max -  average -\n";
            return out.str();
        }

        out << " min " << snapshot.minLength << "  max " << snapshot.maxLength << "  average "
            << std::fixed << std::setprecision(1) << snapshot.averageLength << '\n';
        return out.str();
    }
} // namespace Collector
