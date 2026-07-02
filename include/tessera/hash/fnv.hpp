#pragma once

#include "tessera/core/types.hpp"

#include <cstdint>
#include <string_view>

namespace tessera::hash {

// FNV-1a, the small non-cryptographic hash used for string keys in the index
// and metadata subsystems. Deterministic across platforms; not collision
// resistant against adversarial keys, which is acceptable because the index
// stores and compares the full key on lookup.

inline constexpr std::uint32_t kFnvOffset32 = 0x811C9DC5U;
inline constexpr std::uint32_t kFnvPrime32 = 0x01000193U;
inline constexpr std::uint64_t kFnvOffset64 = 0xCBF29CE484222325ULL;
inline constexpr std::uint64_t kFnvPrime64 = 0x00000100000001B3ULL;

[[nodiscard]] constexpr std::uint32_t fnv1a32(core::ByteSpan data) noexcept {
    std::uint32_t hash = kFnvOffset32;
    for (std::size_t index = 0; index < data.size; ++index) {
        hash ^= static_cast<std::uint32_t>(data.data[index]);
        hash *= kFnvPrime32;
    }
    return hash;
}

[[nodiscard]] constexpr std::uint64_t fnv1a64(core::ByteSpan data) noexcept {
    std::uint64_t hash = kFnvOffset64;
    for (std::size_t index = 0; index < data.size; ++index) {
        hash ^= static_cast<std::uint64_t>(data.data[index]);
        hash *= kFnvPrime64;
    }
    return hash;
}

[[nodiscard]] inline std::uint64_t fnv1a64(std::string_view text) noexcept {
    return fnv1a64(core::ByteSpan{reinterpret_cast<const std::uint8_t*>(text.data()), text.size()});
}

} // namespace tessera::hash
