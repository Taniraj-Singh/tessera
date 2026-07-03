#include "tessera/compress/codec.hpp"
#include "tessera/compress/frame.hpp"
#include "tessera/core/types.hpp"

#include <cstddef>
#include <cstdint>

// Exercises the compression codecs two ways. Path A calls a codec's
// decompressor directly with a chosen declared size. Path B goes through the
// full frame path (magic, CRC, and codec dispatch).
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size >= 5) {
        const std::uint8_t codec_raw = static_cast<std::uint8_t>(data[0] % 4U);
        const std::size_t declared = static_cast<std::size_t>(tessera::core::read_le_u32(data + 1) % (1U << 20U));
        const tessera::core::ByteSpan body{data + 5, size - 5};
        if (auto codec = tessera::compress::parse_codec_id(codec_raw); codec) {
            if (const tessera::compress::Codec* selected = tessera::compress::find_codec(*codec)) {
                (void)selected->decompress(body, declared);
            }
        }
    }

    (void)tessera::compress::decompress_frame(tessera::core::ByteSpan{data, size});
    return 0;
}
