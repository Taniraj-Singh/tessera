#pragma once

#include "tessera/compress/codec.hpp"

namespace tessera::compress {

// Byte-oriented run-length codec in a PackBits-style token stream:
//
//   control C in 0x00..0x7F : literal run of (C + 1) bytes follow verbatim
//   control C in 0x80..0xFF : repeat run of ((C & 0x7F) + 2) copies of one byte
//
// Decompression is bounded by the declared uncompressed size: every token is
// checked against the remaining output budget before it is applied, and the
// final stream must fill the budget exactly.
class RleCodec final : public Codec {
public:
    static constexpr std::size_t kMaxLiteralRun = 128;
    static constexpr std::size_t kMaxRepeatRun = 129;

    [[nodiscard]] CodecId id() const noexcept override { return CodecId::Rle; }
    [[nodiscard]] std::string_view name() const noexcept override { return "rle"; }

    [[nodiscard]] core::Result<std::vector<std::uint8_t>> compress(core::ByteSpan input) const override;
    [[nodiscard]] core::Result<std::vector<std::uint8_t>> decompress(
        core::ByteSpan input,
        std::size_t expected_size) const override;
};

} // namespace tessera::compress
