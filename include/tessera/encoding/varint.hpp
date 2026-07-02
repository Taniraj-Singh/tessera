#pragma once

#include "tessera/buffer/reader_writer.hpp"
#include "tessera/core/types.hpp"
#include "tessera/memory/byte_buffer.hpp"

#include <cstdint>

namespace tessera::encoding {

// LEB128 variable-length integers. The compact framing used by the compression
// and archive layers stores most lengths as uvarints, so a stray high bit in a
// continuation byte can extend a value far past the declared block — the reader
// therefore caps the byte count and reports the offset on failure, matching the
// fail-closed contract of buffer::Reader.

inline constexpr std::size_t kMaxVarint32Bytes = 5;
inline constexpr std::size_t kMaxVarint64Bytes = 10;

[[nodiscard]] core::Result<std::uint32_t> read_uvarint32(buffer::Reader& reader);
[[nodiscard]] core::Result<std::uint64_t> read_uvarint64(buffer::Reader& reader);
[[nodiscard]] core::Result<std::int32_t> read_svarint32(buffer::Reader& reader);
[[nodiscard]] core::Result<std::int64_t> read_svarint64(buffer::Reader& reader);

void write_uvarint32(memory::ByteBuffer& out, std::uint32_t value);
void write_uvarint64(memory::ByteBuffer& out, std::uint64_t value);
void write_svarint32(memory::ByteBuffer& out, std::int32_t value);
void write_svarint64(memory::ByteBuffer& out, std::int64_t value);

// Number of bytes write_uvarint64 would emit for value; used to size frame
// headers before the payload extent is known.
[[nodiscard]] std::size_t uvarint_size(std::uint64_t value) noexcept;

} // namespace tessera::encoding
