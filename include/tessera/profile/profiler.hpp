#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <string>
#include <string_view>

namespace tessera::profile {

// A distribution accumulator: count, sum, and range of observed samples. Time is
// not sampled here — callers feed in whatever measure they track (bytes, rows,
// logical ticks) so the profiler stays deterministic and clock-free.
struct Stat {
    std::uint64_t count{0};
    std::uint64_t sum{0};
    std::uint64_t minimum{std::numeric_limits<std::uint64_t>::max()};
    std::uint64_t maximum{0};

    void observe(std::uint64_t sample) noexcept {
        ++count;
        sum += sample;
        if (sample < minimum) {
            minimum = sample;
        }
        if (sample > maximum) {
            maximum = sample;
        }
    }

    [[nodiscard]] double mean() const noexcept {
        return count == 0 ? 0.0 : static_cast<double>(sum) / static_cast<double>(count);
    }
};

// Named counters plus named sample distributions. Lightweight instrumentation
// for the pipeline and recovery paths; not thread-safe.
class Profiler {
public:
    void count(std::string_view name, std::uint64_t delta = 1);
    void observe(std::string_view name, std::uint64_t sample);

    [[nodiscard]] std::uint64_t counter(std::string_view name) const;
    [[nodiscard]] const Stat* stat(std::string_view name) const;

    [[nodiscard]] std::size_t counter_count() const noexcept { return counters_.size(); }
    [[nodiscard]] std::size_t stat_count() const noexcept { return stats_.size(); }

    void reset() noexcept;
    [[nodiscard]] std::string report() const;

private:
    std::map<std::string, std::uint64_t, std::less<>> counters_;
    std::map<std::string, Stat, std::less<>> stats_;
};

} // namespace tessera::profile
