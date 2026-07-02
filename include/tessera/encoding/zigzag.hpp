#pragma once

#include <cstdint>

namespace tessera::encoding {

// ZigZag maps signed integers onto unsigned so that small-magnitude negatives
// stay small after varint encoding: 0,-1,1,-2,2 -> 0,1,2,3,4. Used by the
// varint codec and the delta-encoded index structures.

[[nodiscard]] constexpr std::uint32_t zigzag_encode32(std::int32_t value) noexcept {
    return (static_cast<std::uint32_t>(value) << 1U) ^ static_cast<std::uint32_t>(value >> 31);
}

[[nodiscard]] constexpr std::int32_t zigzag_decode32(std::uint32_t value) noexcept {
    return static_cast<std::int32_t>((value >> 1U) ^ (~(value & 1U) + 1U));
}

[[nodiscard]] constexpr std::uint64_t zigzag_encode64(std::int64_t value) noexcept {
    return (static_cast<std::uint64_t>(value) << 1U) ^ static_cast<std::uint64_t>(value >> 63);
}

[[nodiscard]] constexpr std::int64_t zigzag_decode64(std::uint64_t value) noexcept {
    return static_cast<std::int64_t>((value >> 1U) ^ (~(value & 1U) + 1U));
}

} // namespace tessera::encoding
