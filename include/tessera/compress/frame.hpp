#pragma once

#include "tessera/buffer/reader_writer.hpp"
#include "tessera/compress/codec.hpp"
#include "tessera/core/types.hpp"

#include <cstdint>
#include <vector>

namespace tessera::compress {

// Self-describing compression envelope. A frame carries its codec, the declared
// uncompressed size, the compressed payload, and a CRC32C over that payload:
//
//   u32     magic (kFrameMagic)
//   u8      codec id
//   u8      flags (reserved, currently 0)
//   uvarint uncompressed size
//   uvarint compressed size
//   bytes   compressed payload
//   u32     crc32c(payload)
//
// The archive and pipeline layers embed frames, so the reader validates the
// magic, bounds both sizes against kMaxBlockSize, checks the CRC, and only then
// hands the payload to the codec — which itself verifies it produced exactly
// the declared number of bytes.

inline constexpr std::uint32_t kFrameMagic = 0x315A4654U; // 'TFZ1' little-endian

struct FrameHeader {
    CodecId codec{CodecId::Store};
    std::uint8_t flags{0};
    std::uint64_t uncompressed_size{0};
    std::uint64_t compressed_size{0};
};

// Parses and validates the fixed portion of a frame, leaving reader positioned
// at the first byte of the compressed payload.
[[nodiscard]] core::Result<FrameHeader> read_frame_header(buffer::Reader& reader);

[[nodiscard]] core::Result<std::vector<std::uint8_t>> compress_frame(core::ByteSpan input, CodecId codec);
[[nodiscard]] core::Result<std::vector<std::uint8_t>> decompress_frame(core::ByteSpan frame);

} // namespace tessera::compress
