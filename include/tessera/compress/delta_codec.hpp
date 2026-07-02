#pragma once

#include "tessera/compress/codec.hpp"

namespace tessera::compress {

// Delta filter followed by run-length coding. Each byte is replaced by its
// difference from the previous byte (modulo 256); slowly varying or monotonic
// data then collapses into long runs of small deltas that the RLE stage
// compresses. Round-trips exactly for any input; falls back to Store at the
// frame layer when it fails to shrink.
class DeltaCodec final : public Codec {
public:
    [[nodiscard]] CodecId id() const noexcept override { return CodecId::Delta; }
    [[nodiscard]] std::string_view name() const noexcept override { return "delta"; }

    [[nodiscard]] core::Result<std::vector<std::uint8_t>> compress(core::ByteSpan input) const override;
    [[nodiscard]] core::Result<std::vector<std::uint8_t>> decompress(
        core::ByteSpan input,
        std::size_t expected_size) const override;
};

} // namespace tessera::compress
