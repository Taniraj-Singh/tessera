#include "tessera/compress/frame.hpp"

#include "tessera/checksum/crc32c.hpp"
#include "tessera/core/constants.hpp"
#include "tessera/encoding/varint.hpp"
#include "tessera/memory/byte_buffer.hpp"

namespace tessera::compress {

core::Result<FrameHeader> read_frame_header(buffer::Reader& reader) {
    FrameHeader header{};

    auto magic = reader.read_u32();
    if (!magic) {
        return magic.error();
    }
    if (*magic != kFrameMagic) {
        return core::Error{core::ErrorCode::InvalidMagic, "invalid frame magic", 0};
    }

    auto codec_raw = reader.read_u8();
    if (!codec_raw) {
        return codec_raw.error();
    }
    auto codec = parse_codec_id(*codec_raw);
    if (!codec) {
        return codec.error();
    }
    header.codec = *codec;

    auto flags = reader.read_u8();
    if (!flags) {
        return flags.error();
    }
    header.flags = *flags;

    auto uncompressed = encoding::read_uvarint64(reader);
    if (!uncompressed) {
        return uncompressed.error();
    }
    if (*uncompressed > core::kMaxBlockSize) {
        return core::Error{core::ErrorCode::InvalidLength, "frame uncompressed size exceeds limit", reader.position()};
    }
    header.uncompressed_size = *uncompressed;

    auto compressed = encoding::read_uvarint64(reader);
    if (!compressed) {
        return compressed.error();
    }
    if (*compressed > core::kMaxBlockSize) {
        return core::Error{core::ErrorCode::InvalidLength, "frame compressed size exceeds limit", reader.position()};
    }
    header.compressed_size = *compressed;

    return header;
}

core::Result<std::vector<std::uint8_t>> compress_frame(core::ByteSpan input, CodecId codec) {
    const Codec* selected = find_codec(codec);
    if (selected == nullptr) {
        return core::Error{core::ErrorCode::InvalidEnum, "unknown codec id", 0};
    }
    if (input.size > core::kMaxBlockSize) {
        return core::Error{core::ErrorCode::InvalidLength, "frame input exceeds limit", 0};
    }

    auto compressed = selected->compress(input);
    if (!compressed) {
        return compressed.error();
    }

    // Fall back to Store when the transform did not shrink the payload, so a
    // frame never costs more than the raw bytes plus its header.
    CodecId chosen = codec;
    std::vector<std::uint8_t> payload = std::move(*compressed);
    if (chosen != CodecId::Store && payload.size() >= input.size) {
        chosen = CodecId::Store;
        payload.assign(input.begin(), input.end());
    }

    memory::ByteBuffer out(payload.size() + 16);
    out.append_u32(kFrameMagic);
    out.append_u8(static_cast<std::uint8_t>(chosen));
    out.append_u8(0);
    encoding::write_uvarint64(out, input.size);
    encoding::write_uvarint64(out, payload.size());
    out.append_bytes(core::ByteSpan{payload.data(), payload.size()});
    out.append_u32(checksum::crc32c(core::ByteSpan{payload.data(), payload.size()}));
    return out.take();
}

core::Result<std::vector<std::uint8_t>> decompress_frame(core::ByteSpan frame) {
    buffer::Reader reader(frame);
    auto header = read_frame_header(reader);
    if (!header) {
        return header.error();
    }

    auto payload = reader.read_span(static_cast<std::size_t>(header->compressed_size));
    if (!payload) {
        return payload.error();
    }

    auto stored_crc = reader.read_u32();
    if (!stored_crc) {
        return stored_crc.error();
    }
    if (auto verified = checksum::verify(*payload, *stored_crc); !verified) {
        return verified.error();
    }

    const Codec* selected = find_codec(header->codec);
    if (selected == nullptr) {
        return core::Error{core::ErrorCode::InvalidEnum, "unknown codec id", 0};
    }
    return selected->decompress(*payload, static_cast<std::size_t>(header->uncompressed_size));
}

} // namespace tessera::compress
