#include "tessera/index/ordered_index.hpp"

#include <algorithm>

namespace tessera::index {

void OrderedIndex::insert(std::uint64_t key, std::uint32_t value) {
    entries_.emplace_back(key, value);
    sorted_ = false;
}

void OrderedIndex::build() {
    std::sort(entries_.begin(), entries_.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });
    sorted_ = true;
}

std::size_t OrderedIndex::lower_bound(std::uint64_t key) const {
    std::size_t low = 0;
    std::size_t high = entries_.size();
    while (low < high) {
        const std::size_t mid = low + (high - low) / 2;
        if (entries_[mid].first < key) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    return low;
}

const std::uint32_t* OrderedIndex::find(std::uint64_t key) const {
    if (!sorted_) {
        return nullptr;
    }
    const std::size_t index = lower_bound(key);
    if (index < entries_.size() && entries_[index].first == key) {
        return &entries_[index].second;
    }
    return nullptr;
}

std::vector<std::uint32_t> OrderedIndex::range(std::uint64_t low, std::uint64_t high) const {
    std::vector<std::uint32_t> result;
    if (!sorted_ || low > high) {
        return result;
    }
    for (std::size_t index = lower_bound(low); index < entries_.size() && entries_[index].first <= high; ++index) {
        result.push_back(entries_[index].second);
    }
    return result;
}

void OrderedIndex::clear() noexcept {
    entries_.clear();
    sorted_ = true;
}

} // namespace tessera::index
