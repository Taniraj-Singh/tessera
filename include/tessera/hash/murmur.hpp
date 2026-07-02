#pragma once

#include "tessera/core/types.hpp"

#include <cstdint>

namespace tessera::hash {

// MurmurHash3 (x86 32-bit variant) plus the 64-bit finalizer. Used to spread
// integer keys across the open-addressing table so that sequential section ids
// do not cluster into long probe chains.

// Avalanche finalizers — bijective bit mixers with strong avalanche.
[[nodiscard]] constexpr std::uint32_t fmix32(std::uint32_t value) noexcept {
    value ^= value >> 16U;
    value *= 0x85EBCA6BU;
    value ^= value >> 13U;
    value *= 0xC2B2AE35U;
    value ^= value >> 16U;
    return value;
}

[[nodiscard]] constexpr std::uint64_t fmix64(std::uint64_t value) noexcept {
    value ^= value >> 33U;
    value *= 0xFF51AFD7ED558CCDULL;
    value ^= value >> 33U;
    value *= 0xC4CEB9FE1A85EC53ULL;
    value ^= value >> 33U;
    return value;
}

[[nodiscard]] std::uint32_t murmur3_x86_32(core::ByteSpan data, std::uint32_t seed) noexcept;

} // namespace tessera::hash
