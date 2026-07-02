#include "tessera/compress/rle_codec.hpp"

#include "tessera/buffer/reader_writer.hpp"
#include "tessera/memory/byte_buffer.hpp"

namespace tessera::compress {

namespace {

// Length of the identical-byte run starting at index, capped at limit.
std::size_t run_length(core::ByteSpan input, std::size_t index, std::size_t limit) noexcept {
    std::size_t run = 1;
    while (index + run < input.size && input.data[index + run] == input.data[index] && run < limit) {
        ++run;
    }
    return run;
}

} // namespace

core::Result<std::vector<std::uint8_t>> RleCodec::compress(core::ByteSpan input) const {
    memory::ByteBuffer out(input.size);
    std::size_t index = 0;
    while (index < input.size) {
        const std::size_t run = run_length(input, index, kMaxRepeatRun);
        if (run >= 3) {
            out.append_u8(static_cast<std::uint8_t>(0x80U | (run - 2U)));
            out.append_u8(input.data[index]);
            index += run;
            continue;
        }

        // Accumulate a literal chunk until a compressible run appears or the
        // per-token cap is reached.
        const std::size_t literal_start = index;
        std::size_t literal_count = 0;
        while (index < input.size && literal_count < kMaxLiteralRun) {
            if (run_length(input, index, 3) >= 3) {
                break;
            }
            ++index;
            ++literal_count;
        }
        out.append_u8(static_cast<std::uint8_t>(literal_count - 1U));
        out.append_bytes(core::ByteSpan{input.data + literal_start, literal_count});
    }
    return out.take();
}

core::Result<std::vector<std::uint8_t>> RleCodec::decompress(
    core::ByteSpan input,
    std::size_t expected_size) const {
    buffer::Reader reader(input);
    std::vector<std::uint8_t> out;
    out.reserve(expected_size);

    while (reader.remaining() > 0) {
        auto control = reader.read_u8();
        if (!control) {
            return control.error();
        }
        if (*control < 0x80U) {
            const std::size_t length = static_cast<std::size_t>(*control) + 1U;
            if (out.size() + length > expected_size) {
                return core::Error{core::ErrorCode::InvalidLength, "rle literal exceeds declared size", reader.position()};
            }
            auto bytes = reader.read_bytes(length);
            if (!bytes) {
                return bytes.error();
            }
            out.insert(out.end(), bytes->begin(), bytes->end());
        } else {
            const std::size_t length = static_cast<std::size_t>(*control & 0x7FU) + 2U;
            auto value = reader.read_u8();
            if (!value) {
                return value.error();
            }
            if (out.size() + length > expected_size) {
                return core::Error{core::ErrorCode::InvalidLength, "rle run exceeds declared size", reader.position()};
            }
            out.insert(out.end(), length, *value);
        }
    }

    if (out.size() != expected_size) {
        return core::Error{core::ErrorCode::InvalidLength, "rle output size mismatch", 0};
    }
    return out;
}

} // namespace tessera::compress
