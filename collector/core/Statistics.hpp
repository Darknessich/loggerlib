#pragma once

#include <logger/LogLevel.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace Collector {
    using TClock = std::chrono::steady_clock;

    struct SSnapshot {
        std::size_t total{0};
        std::size_t malformed{0};
        std::array<std::size_t, static_cast<std::size_t>(Logger::ELogLevel::Count)> byLevel{};
        std::size_t lastHour{0};
        std::size_t minLength{0};
        std::size_t maxLength{0};
        double averageLength{0.0};
    };

    class Statistics {
    public:
        void add(Logger::ELogLevel level, std::size_t length, TClock::time_point now) noexcept;
        void addMalformed() noexcept;

        [[nodiscard]] SSnapshot snapshot(TClock::time_point now) const noexcept;

    private:
        struct SBucket {
            std::int64_t minute{-1};
            std::size_t count{0};
        };

        static constexpr std::size_t kBuckets = 60;

        std::array<SBucket, kBuckets> m_minutes{};
        decltype(SSnapshot::byLevel) m_byLevel{};

        std::size_t m_total{0};
        std::size_t m_malformed{0};
        std::size_t m_minLength{0};
        std::size_t m_maxLength{0};
        std::uint64_t m_totalLength{0};
    };

    std::string formatReport(const SSnapshot& snapshot, std::string_view reason);
} // namespace Collector
