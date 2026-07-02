#include "tessera/compress/lz77_codec.hpp"

#include "tessera/buffer/reader_writer.hpp"
#include "tessera/memory/byte_buffer.hpp"

namespace tessera::compress {

namespace {

struct Match {
    std::size_t length{0};
    std::size_t distance{0};
};

// Naive greedy longest-match search over the preceding window. Adequate for the
// container's modest section sizes; the format it produces is what the decoder
// (and any adversarial input) must be validated against.
Match find_match(core::ByteSpan input, std::size_t cursor) {
    Match best{};
    const std::size_t window_start = cursor > Lz77Codec::kWindowSize ? cursor - Lz77Codec::kWindowSize : 0;
    for (std::size_t candidate = window_start; candidate < cursor; ++candidate) {
        std::size_t length = 0;
        while (length < Lz77Codec::kMaxMatch &&
               cursor + length < input.size &&
               input.data[candidate + length] == input.data[cursor + length]) {
            ++length;
        }
        if (length > best.length) {
            best.length = length;
            best.distance = cursor - candidate;
        }
    }
    return best;
}

} // namespace

core::Result<std::vector<std::uint8_t>> Lz77Codec::compress(core::ByteSpan input) const {
    memory::ByteBuffer out(input.size);
    std::size_t cursor = 0;
    while (cursor < input.size) {
        const std::size_t tag_position = out.size();
        out.append_u8(0);
        std::uint8_t tag = 0;

        for (int bit = 0; bit < 8 && cursor < input.size; ++bit) {
            const Match match = find_match(input, cursor);
            if (match.length >= kMinMatch) {
                tag = static_cast<std::uint8_t>(tag | (1U << static_cast<unsigned>(bit)));
                const std::uint16_t word = static_cast<std::uint16_t>(
                    ((match.distance - 1U) << 4U) | (match.length - kMinMatch));
                out.append_u16(word);
                cursor += match.length;
            } else {
                out.append_u8(input.data[cursor]);
                ++cursor;
            }
        }

        out.data()[tag_position] = tag;
    }
    return out.take();
}

core::Result<std::vector<std::uint8_t>> Lz77Codec::decompress(
    core::ByteSpan input,
    std::size_t expected_size) const {
    buffer::Reader reader(input);
    std::vector<std::uint8_t> out;
    out.reserve(expected_size);

    while (out.size() < expected_size) {
        auto tag = reader.read_u8();
        if (!tag) {
            return tag.error();
        }
        for (int bit = 0; bit < 8 && out.size() < expected_size; ++bit) {
            const bool is_match = ((*tag >> static_cast<unsigned>(bit)) & 1U) != 0;
            if (!is_match) {
                auto literal = reader.read_u8();
                if (!literal) {
                    return literal.error();
                }
                out.push_back(*literal);
                continue;
            }

            auto low = reader.read_u8();
            if (!low) {
                return low.error();
            }
            auto high = reader.read_u8();
            if (!high) {
                return high.error();
            }
            const std::uint16_t word = static_cast<std::uint16_t>(*low | (static_cast<unsigned>(*high) << 8U));
            const std::size_t distance = (static_cast<std::size_t>(word) >> 4U) + 1U;
            const std::size_t length = static_cast<std::size_t>(word & 0x0FU) + kMinMatch;

            if (distance > out.size()) {
                return core::Error{core::ErrorCode::InvalidLength, "lz77 back-reference before output start", reader.position()};
            }
            if (out.size() + length > expected_size) {
                return core::Error{core::ErrorCode::InvalidLength, "lz77 match exceeds declared size", reader.position()};
            }
            for (std::size_t copied = 0; copied < length; ++copied) {
                out.push_back(out[out.size() - distance]);
            }
        }
    }

    if (out.size() != expected_size) {
        return core::Error{core::ErrorCode::InvalidLength, "lz77 output size mismatch", 0};
    }
    return out;
}

} // namespace tessera::compress
