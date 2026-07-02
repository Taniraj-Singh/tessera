#pragma once

#include "tessera/compress/codec.hpp"

namespace tessera::compress {

// Identity codec. compress() copies input verbatim; decompress() validates that
// the stored length matches the declared uncompressed size. Used as the
// fallback when a payload does not compress and as a baseline in round-trip
// tests.
class StoreCodec final : public Codec {
public:
    [[nodiscard]] CodecId id() const noexcept override { return CodecId::Store; }
    [[nodiscard]] std::string_view name() const noexcept override { return "store"; }

    [[nodiscard]] core::Result<std::vector<std::uint8_t>> compress(core::ByteSpan input) const override;
    [[nodiscard]] core::Result<std::vector<std::uint8_t>> decompress(
        core::ByteSpan input,
        std::size_t expected_size) const override;
};

} // namespace tessera::compress
