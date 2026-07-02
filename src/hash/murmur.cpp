#include "tessera/hash/murmur.hpp"

#include "tessera/util/bit_ops.hpp"

namespace tessera::hash {

std::uint32_t murmur3_x86_32(core::ByteSpan data, std::uint32_t seed) noexcept {
    constexpr std::uint32_t c1 = 0xCC9E2D51U;
    constexpr std::uint32_t c2 = 0x1B873593U;

    std::uint32_t hash = seed;
    const std::size_t block_count = data.size / 4;

    for (std::size_t block = 0; block < block_count; ++block) {
        const std::size_t offset = block * 4;
        std::uint32_t k = core::read_le_u32(data.data + offset);
        k *= c1;
        k = util::rotl32(k, 15);
        k *= c2;
        hash ^= k;
        hash = util::rotl32(hash, 13);
        hash = hash * 5U + 0xE6546B64U;
    }

    // Tail — up to three trailing bytes that do not form a full block.
    std::uint32_t tail = 0;
    const std::size_t tail_start = block_count * 4;
    const std::size_t tail_size = data.size & 3U;
    if (tail_size >= 3) {
        tail ^= static_cast<std::uint32_t>(data.data[tail_start + 2]) << 16U;
    }
    if (tail_size >= 2) {
        tail ^= static_cast<std::uint32_t>(data.data[tail_start + 1]) << 8U;
    }
    if (tail_size >= 1) {
        tail ^= static_cast<std::uint32_t>(data.data[tail_start + 0]);
        tail *= c1;
        tail = util::rotl32(tail, 15);
        tail *= c2;
        hash ^= tail;
    }

    hash ^= static_cast<std::uint32_t>(data.size);
    return fmix32(hash);
}

} // namespace tessera::hash
