#include "tessera/encoding/varint.hpp"

#include "tessera/encoding/zigzag.hpp"

namespace tessera::encoding {

namespace {

// Shared decode loop bounded to max_bytes continuation groups. A group whose
// terminal byte still has the continuation bit set, or a value that overflows
// the target width, is rejected rather than silently truncated.
core::Result<std::uint64_t> read_bounded(buffer::Reader& reader, std::size_t max_bytes) {
    std::uint64_t result = 0;
    for (std::size_t index = 0; index < max_bytes; ++index) {
        auto byte = reader.read_u8();
        if (!byte) {
            return byte.error();
        }
        const std::uint64_t payload = static_cast<std::uint64_t>(*byte & 0x7FU);
        const unsigned shift = static_cast<unsigned>(index) * 7U;
        // On the final permitted byte only the bits that fit in 64 remain valid.
        if (index + 1 == max_bytes) {
            const unsigned remaining = 64U - shift;
            if (remaining < 7U && (payload >> remaining) != 0) {
                return core::Error{core::ErrorCode::InvalidLength, "varint overflows 64 bits", reader.position()};
            }
        }
        result |= payload << shift;
        if ((*byte & 0x80U) == 0) {
            return result;
        }
    }
    return core::Error{core::ErrorCode::InvalidLength, "varint missing terminator", reader.position()};
}

} // namespace

core::Result<std::uint64_t> read_uvarint64(buffer::Reader& reader) {
    return read_bounded(reader, kMaxVarint64Bytes);
}

core::Result<std::uint32_t> read_uvarint32(buffer::Reader& reader) {
    auto wide = read_bounded(reader, kMaxVarint32Bytes);
    if (!wide) {
        return wide.error();
    }
    if (*wide > 0xFFFFFFFFULL) {
        return core::Error{core::ErrorCode::InvalidLength, "varint overflows 32 bits", reader.position()};
    }
    return static_cast<std::uint32_t>(*wide);
}

core::Result<std::int64_t> read_svarint64(buffer::Reader& reader) {
    auto raw = read_uvarint64(reader);
    if (!raw) {
        return raw.error();
    }
    return zigzag_decode64(*raw);
}

core::Result<std::int32_t> read_svarint32(buffer::Reader& reader) {
    auto raw = read_uvarint32(reader);
    if (!raw) {
        return raw.error();
    }
    return zigzag_decode32(*raw);
}

void write_uvarint64(memory::ByteBuffer& out, std::uint64_t value) {
    while (value >= 0x80U) {
        out.append_u8(static_cast<std::uint8_t>((value & 0x7FU) | 0x80U));
        value >>= 7U;
    }
    out.append_u8(static_cast<std::uint8_t>(value & 0x7FU));
}

void write_uvarint32(memory::ByteBuffer& out, std::uint32_t value) {
    write_uvarint64(out, value);
}

void write_svarint64(memory::ByteBuffer& out, std::int64_t value) {
    write_uvarint64(out, zigzag_encode64(value));
}

void write_svarint32(memory::ByteBuffer& out, std::int32_t value) {
    write_uvarint64(out, zigzag_encode32(value));
}

std::size_t uvarint_size(std::uint64_t value) noexcept {
    std::size_t bytes = 1;
    while (value >= 0x80U) {
        value >>= 7U;
        ++bytes;
    }
    return bytes;
}

} // namespace tessera::encoding
