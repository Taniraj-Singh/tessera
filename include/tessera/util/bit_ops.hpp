#pragma once

#include <cstddef>
#include <cstdint>

namespace tessera::util {

// Small, dependency-free bit helpers shared by the memory, index, and
// compression subsystems. All functions are constexpr and total (defined for
// every input in their domain, including zero).

[[nodiscard]] constexpr int popcount(std::uint64_t value) noexcept {
    int count = 0;
    while (value != 0) {
        value &= value - 1U;
        ++count;
    }
    return count;
}

[[nodiscard]] constexpr int count_leading_zeros(std::uint64_t value) noexcept {
    if (value == 0) {
        return 64;
    }
    int count = 0;
    for (int bit = 63; bit >= 0; --bit) {
        if ((value >> static_cast<unsigned>(bit)) & 1U) {
            break;
        }
        ++count;
    }
    return count;
}

[[nodiscard]] constexpr int count_trailing_zeros(std::uint64_t value) noexcept {
    if (value == 0) {
        return 64;
    }
    int count = 0;
    while ((value & 1U) == 0) {
        value >>= 1U;
        ++count;
    }
    return count;
}

[[nodiscard]] constexpr bool is_power_of_two(std::uint64_t value) noexcept {
    return value != 0 && (value & (value - 1U)) == 0;
}

// Smallest power of two that is >= value. Returns 1 for value == 0 and
// saturates at 2^63 to avoid wrapping to zero.
[[nodiscard]] constexpr std::uint64_t next_power_of_two(std::uint64_t value) noexcept {
    if (value <= 1U) {
        return 1U;
    }
    const std::uint64_t highest = 1ULL << 63U;
    if (value > highest) {
        return highest;
    }
    std::uint64_t result = 1U;
    while (result < value) {
        result <<= 1U;
    }
    return result;
}

// Round size up to the next multiple of alignment. alignment must be a power of
// two; callers guarantee this (allocators pass compile-time constants).
[[nodiscard]] constexpr std::size_t align_up(std::size_t size, std::size_t alignment) noexcept {
    const std::size_t mask = alignment - 1U;
    return (size + mask) & ~mask;
}

// Rotate helpers used by the murmur/fnv mixing in the hash subsystem.
[[nodiscard]] constexpr std::uint32_t rotl32(std::uint32_t value, unsigned shift) noexcept {
    shift &= 31U;
    return (value << shift) | (value >> ((32U - shift) & 31U));
}

[[nodiscard]] constexpr std::uint64_t rotl64(std::uint64_t value, unsigned shift) noexcept {
    shift &= 63U;
    return (value << shift) | (value >> ((64U - shift) & 63U));
}

} // namespace tessera::util
