#pragma once

#include "tessera/compress/codec.hpp"

namespace tessera::compress {

// LZSS sliding-window codec. The token stream is grouped: a tag byte precedes
// up to eight items, its bits (LSB first) selecting literal (0) or match (1)
// for each. A match is a little-endian 16-bit word packing a 12-bit distance
// and a 4-bit length:
//
//   distance = (word >> 4) + 1     range [1, 4096]
//   length   = (word & 0x0F) + 3   range [3, 18]
//
// Decompression is driven by the declared uncompressed size and copies each
// back-reference from already-produced output, so a match must never name a
// distance larger than the current output or a length that would overrun the
// declared size.
class Lz77Codec final : public Codec {
public:
    static constexpr std::size_t kWindowSize = 4096;
    static constexpr std::size_t kMinMatch = 3;
    static constexpr std::size_t kMaxMatch = 18;

    [[nodiscard]] CodecId id() const noexcept override { return CodecId::Lz77; }
    [[nodiscard]] std::string_view name() const noexcept override { return "lz77"; }

    [[nodiscard]] core::Result<std::vector<std::uint8_t>> compress(core::ByteSpan input) const override;
    [[nodiscard]] core::Result<std::vector<std::uint8_t>> decompress(
        core::ByteSpan input,
        std::size_t expected_size) const override;
};

} // namespace tessera::compress
