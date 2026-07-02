#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace tessera::index {

// Sorted key/value index supporting point and range lookups. Unlike the
// open-addressing HashIndex it preserves order, so it answers "all values whose
// key falls in [low, high]" — used for range scans over section ids and ordered
// record keys. Insertions are buffered and sorted lazily on build().
class OrderedIndex {
public:
    void insert(std::uint64_t key, std::uint32_t value);
    void build();

    [[nodiscard]] const std::uint32_t* find(std::uint64_t key) const;
    [[nodiscard]] bool contains(std::uint64_t key) const { return find(key) != nullptr; }

    // Values whose keys lie in the inclusive range [low, high], in key order.
    [[nodiscard]] std::vector<std::uint32_t> range(std::uint64_t low, std::uint64_t high) const;

    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    [[nodiscard]] bool empty() const noexcept { return entries_.empty(); }
    [[nodiscard]] bool built() const noexcept { return sorted_; }

    void clear() noexcept;

private:
    [[nodiscard]] std::size_t lower_bound(std::uint64_t key) const;

    std::vector<std::pair<std::uint64_t, std::uint32_t>> entries_;
    bool sorted_{true};
};

} // namespace tessera::index
