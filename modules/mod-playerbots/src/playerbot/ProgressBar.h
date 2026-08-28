#pragma once

#include "Log.h"

#include <atomic>
#include <cstdint>
#include <limits>

// Module-owned replacement for CMaNGOS BarGoLink. Penqle has no console
// progress-bar helper, so keep the playerbot feature local to the module
// instead of pretending it exists through a compatibility no-op.
class BarGoLink
{
public:
    BarGoLink() = default;

    template <typename T>
    explicit BarGoLink(T total) : total_(static_cast<std::uint64_t>(total))
    {
    }

    void step()
    {
        if (current_ < std::numeric_limits<std::uint64_t>::max())
            ++current_;

        if (!outputEnabled_.load(std::memory_order_relaxed) || !total_)
            return;

        std::uint32_t percent = static_cast<std::uint32_t>((current_ * 100) / total_);
        if (percent < nextReport_ && current_ < total_)
            return;

        sLog.outString("Playerbots progress: %u%% (%llu/%llu)", percent,
            static_cast<unsigned long long>(current_),
            static_cast<unsigned long long>(total_));

        nextReport_ = percent >= 100 ? 101 : ((percent / 5) + 1) * 5;
    }

    void Step() { step(); }

    static void SetOutputState(bool state)
    {
        outputEnabled_.store(state, std::memory_order_relaxed);
    }

private:
    inline static std::atomic_bool outputEnabled_{false};
    std::uint64_t total_ = 0;
    std::uint64_t current_ = 0;
    std::uint32_t nextReport_ = 0;
};
