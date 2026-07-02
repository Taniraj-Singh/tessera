#include "tessera/index/string_index.hpp"

#include "tessera/hash/fnv.hpp"
#include "tessera/hash/murmur.hpp"
#include "tessera/util/bit_ops.hpp"

namespace tessera::index {

StringIndex::StringIndex(std::size_t initial_capacity) {
    const std::size_t capacity = static_cast<std::size_t>(
        util::next_power_of_two(initial_capacity < kMinCapacity ? kMinCapacity : initial_capacity));
    slots_.assign(capacity, Slot{});
}

std::size_t StringIndex::probe_start(std::string_view key) const noexcept {
    const std::uint64_t mixed = hash::fmix64(hash::fnv1a64(key));
    return static_cast<std::size_t>(mixed) & (slots_.size() - 1U);
}

const std::uint32_t* StringIndex::find(std::string_view key) const {
    const std::size_t mask = slots_.size() - 1U;
    std::size_t index = probe_start(key);
    for (std::size_t step = 0; step < slots_.size(); ++step) {
        const Slot& slot = slots_[index];
        if (!slot.occupied) {
            return nullptr;
        }
        if (slot.key == key) {
            return &slot.value;
        }
        index = (index + 1U) & mask;
    }
    return nullptr;
}

bool StringIndex::insert(std::string_view key, std::uint32_t value) {
    if ((live_ + 1U) * kLoadDenominator >= slots_.size() * kLoadNumerator) {
        const std::size_t next = slots_.size() << 1U;
        if (next < slots_.size()) {
            return false;
        }
        rehash(next);
    }

    const std::size_t mask = slots_.size() - 1U;
    std::size_t index = probe_start(key);
    for (std::size_t step = 0; step < slots_.size(); ++step) {
        Slot& slot = slots_[index];
        if (!slot.occupied) {
            slot.key.assign(key);
            slot.value = value;
            slot.occupied = true;
            ++live_;
            return true;
        }
        if (slot.key == key) {
            slot.value = value;
            return true;
        }
        index = (index + 1U) & mask;
    }
    return false;
}

void StringIndex::rehash(std::size_t new_capacity) {
    std::vector<Slot> old = std::move(slots_);
    slots_.assign(new_capacity, Slot{});
    live_ = 0;
    for (Slot& slot : old) {
        if (slot.occupied) {
            insert(slot.key, slot.value);
        }
    }
}

void StringIndex::clear() {
    for (Slot& slot : slots_) {
        slot = Slot{};
    }
    live_ = 0;
}

} // namespace tessera::index
