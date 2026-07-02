#pragma once

#include "tessera/core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace tessera::index {

// Open-addressing hash table mapping a 64-bit key to a 32-bit value (typically
// a section slot). Linear probing with tombstones and power-of-two capacity;
// keys are avalanche-mixed before masking so sequential ids spread evenly.
//
// find() returns a pointer to the stored value or nullptr — no allocation, no
// exceptions. The table grows when the live load factor crosses the threshold.
class HashIndex {
public:
    static constexpr std::size_t kMinCapacity = 8;
    // Grow once occupancy (live + tombstones) reaches this fraction of capacity,
    // scaled by 16 to stay in integer arithmetic (11/16 ≈ 0.69).
    static constexpr std::size_t kLoadNumerator = 11;
    static constexpr std::size_t kLoadDenominator = 16;

    HashIndex() : HashIndex(kMinCapacity) {}
    explicit HashIndex(std::size_t initial_capacity);

    // Inserts or overwrites the value for key. Returns false only if a rehash
    // would be required but the requested capacity overflows.
    bool insert(std::uint64_t key, std::uint32_t value);

    [[nodiscard]] const std::uint32_t* find(std::uint64_t key) const;
    [[nodiscard]] bool contains(std::uint64_t key) const { return find(key) != nullptr; }

    bool erase(std::uint64_t key);

    [[nodiscard]] std::size_t size() const noexcept { return live_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return slots_.size(); }
    [[nodiscard]] bool empty() const noexcept { return live_ == 0; }

    void clear();

private:
    enum class State : std::uint8_t { Empty, Occupied, Tombstone };

    struct Slot {
        std::uint64_t key{0};
        std::uint32_t value{0};
        State state{State::Empty};
    };

    [[nodiscard]] std::size_t probe_start(std::uint64_t key) const noexcept;
    void rehash(std::size_t new_capacity);

    std::vector<Slot> slots_;
    std::size_t live_{0};
    std::size_t occupied_{0}; // live + tombstones, drives growth
};

} // namespace tessera::index
