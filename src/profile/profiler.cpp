#include "tessera/profile/profiler.hpp"

#include <sstream>

namespace tessera::profile {

void Profiler::count(std::string_view name, std::uint64_t delta) {
    auto it = counters_.find(name);
    if (it == counters_.end()) {
        counters_.emplace(std::string(name), delta);
    } else {
        it->second += delta;
    }
}

void Profiler::observe(std::string_view name, std::uint64_t sample) {
    auto it = stats_.find(name);
    if (it == stats_.end()) {
        Stat stat{};
        stat.observe(sample);
        stats_.emplace(std::string(name), stat);
    } else {
        it->second.observe(sample);
    }
}

std::uint64_t Profiler::counter(std::string_view name) const {
    auto it = counters_.find(name);
    return it == counters_.end() ? 0U : it->second;
}

const Stat* Profiler::stat(std::string_view name) const {
    auto it = stats_.find(name);
    return it == stats_.end() ? nullptr : &it->second;
}

void Profiler::reset() noexcept {
    counters_.clear();
    stats_.clear();
}

std::string Profiler::report() const {
    std::ostringstream out;
    for (const auto& [name, value] : counters_) {
        out << "counter " << name << " = " << value << '\n';
    }
    for (const auto& [name, stat] : stats_) {
        out << "stat " << name << " count=" << stat.count << " sum=" << stat.sum
            << " min=" << (stat.count == 0 ? 0U : stat.minimum) << " max=" << stat.maximum << '\n';
    }
    return out.str();
}

} // namespace tessera::profile
