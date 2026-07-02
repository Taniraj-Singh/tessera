#include "tessera/index/hash_index.hpp"

#include "tessera/hash/murmur.hpp"
#include "tessera/util/bit_ops.hpp"

namespace tessera::index {

HashIndex::HashIndex(std::size_t initial_capacity) {
    const std::size_t capacity = static_cast<std::size_t>(
        util::next_power_of_two(initial_capacity < kMinCapacity ? kMinCapacity : initial_capacity));
    slots_.assign(capacity, Slot{});
}

std::size_t HashIndex::probe_start(std::uint64_t key) const noexcept {
    const std::uint64_t mixed = hash::fmix64(key);
    return static_cast<std::size_t>(mixed) & (slots_.size() - 1U);
}

const std::uint32_t* HashIndex::find(std::uint64_t key) const {
    const std::size_t mask = slots_.size() - 1U;
    std::size_t index = probe_start(key);
    for (std::size_t step = 0; step < slots_.size(); ++step) {
        const Slot& slot = slots_[index];
        if (slot.state == State::Empty) {
            return nullptr;
        }
        if (slot.state == State::Occupied && slot.key == key) {
            return &slot.value;
        }
        index = (index + 1U) & mask;
    }
    return nullptr;
}

bool HashIndex::insert(std::uint64_t key, std::uint32_t value) {
    // Grow before insertion when occupancy (including tombstones) is high so
    // probe chains stay short and lookups terminate on an Empty slot.
    if ((occupied_ + 1U) * kLoadDenominator >= slots_.size() * kLoadNumerator) {
        const std::size_t next = slots_.size() << 1U;
        if (next < slots_.size()) {
            return false; // capacity overflow
        }
        rehash(next);
    }

    const std::size_t mask = slots_.size() - 1U;
    std::size_t index = probe_start(key);
    std::size_t first_tombstone = slots_.size();
    for (std::size_t step = 0; step < slots_.size(); ++step) {
        Slot& slot = slots_[index];
        if (slot.state == State::Empty) {
            // Reuse an earlier tombstone if one was seen on this probe chain.
            Slot& target = first_tombstone == slots_.size() ? slot : slots_[first_tombstone];
            if (first_tombstone == slots_.size()) {
                ++occupied_;
            }
            target.key = key;
            target.value = value;
            target.state = State::Occupied;
            ++live_;
            return true;
        }
        if (slot.state == State::Tombstone) {
            if (first_tombstone == slots_.size()) {
                first_tombstone = index;
            }
        } else if (slot.key == key) {
            slot.value = value; // overwrite existing key in place
            return true;
        }
        index = (index + 1U) & mask;
    }
    return false;
}

bool HashIndex::erase(std::uint64_t key) {
    const std::size_t mask = slots_.size() - 1U;
    std::size_t index = probe_start(key);
    for (std::size_t step = 0; step < slots_.size(); ++step) {
        Slot& slot = slots_[index];
        if (slot.state == State::Empty) {
            return false;
        }
        if (slot.state == State::Occupied && slot.key == key) {
            slot.state = State::Tombstone;
            --live_;
            return true;
        }
        index = (index + 1U) & mask;
    }
    return false;
}

void HashIndex::rehash(std::size_t new_capacity) {
    std::vector<Slot> old = std::move(slots_);
    slots_.assign(new_capacity, Slot{});
    live_ = 0;
    occupied_ = 0;
    for (const Slot& slot : old) {
        if (slot.state == State::Occupied) {
            insert(slot.key, slot.value);
        }
    }
}

void HashIndex::clear() {
    for (Slot& slot : slots_) {
        slot = Slot{};
    }
    live_ = 0;
    occupied_ = 0;
}

} // namespace tessera::index
