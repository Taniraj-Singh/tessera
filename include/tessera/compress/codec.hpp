#pragma once

#include "tessera/core/types.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace tessera::compress {

// Identifier persisted in a frame header. Values are stable across versions;
// unknown ids are rejected during frame parsing rather than treated as Store.
enum class CodecId : std::uint8_t {
    Store = 0, // identity — no transform
    Rle = 1,   // byte-oriented run-length (PackBits-style)
    Lz77 = 2,  // LZSS sliding window with 12-bit distance / 4-bit length
    Delta = 3, // byte-delta filter followed by run-length coding
};

// A codec transforms a byte range in both directions. Decompression takes the
// declared uncompressed size so the implementation can pre-size its output and
// verify the stream produced exactly that many bytes — an under- or over-run is
// a structural error, never a silent truncation.
class Codec {
public:
    virtual ~Codec() = default;

    [[nodiscard]] virtual CodecId id() const noexcept = 0;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    [[nodiscard]] virtual core::Result<std::vector<std::uint8_t>> compress(core::ByteSpan input) const = 0;
    [[nodiscard]] virtual core::Result<std::vector<std::uint8_t>> decompress(
        core::ByteSpan input,
        std::size_t expected_size) const = 0;
};

// Validates a raw codec byte read from a frame header.
[[nodiscard]] core::Result<CodecId> parse_codec_id(std::uint8_t raw);

// Returns the process-wide singleton for a codec, or nullptr if unregistered.
// The returned pointer is stable for the lifetime of the program.
[[nodiscard]] const Codec* find_codec(CodecId id) noexcept;

[[nodiscard]] std::string_view codec_name(CodecId id) noexcept;

} // namespace tessera::compress
