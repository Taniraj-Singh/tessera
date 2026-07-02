#include "tessera/compress/delta_codec.hpp"

#include "tessera/compress/rle_codec.hpp"

namespace tessera::compress {

core::Result<std::vector<std::uint8_t>> DeltaCodec::compress(core::ByteSpan input) const {
    std::vector<std::uint8_t> deltas(input.size);
    std::uint8_t previous = 0;
    for (std::size_t index = 0; index < input.size; ++index) {
        deltas[index] = static_cast<std::uint8_t>(input.data[index] - previous);
        previous = input.data[index];
    }
    const RleCodec rle;
    return rle.compress(core::ByteSpan{deltas.data(), deltas.size()});
}

core::Result<std::vector<std::uint8_t>> DeltaCodec::decompress(core::ByteSpan input, std::size_t expected_size) const {
    const RleCodec rle;
    auto deltas = rle.decompress(input, expected_size);
    if (!deltas) {
        return deltas.error();
    }
    std::vector<std::uint8_t> output(deltas->size());
    std::uint8_t previous = 0;
    for (std::size_t index = 0; index < deltas->size(); ++index) {
        previous = static_cast<std::uint8_t>((*deltas)[index] + previous);
        output[index] = previous;
    }
    return output;
}

} // namespace tessera::compress
