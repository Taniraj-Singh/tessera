#include "tessera/compress/codec.hpp"
#include "tessera/compress/frame.hpp"
#include "tessera/core/types.hpp"

#include <cstddef>
#include <cstdint>

// Exercises the compression codecs from two directions. Path A drives a codec's
// decompressor directly with an attacker-chosen declared size, reaching the
// back-reference and run-length logic without first satisfying the frame's
// magic/CRC gate. Path B validates the full self-describing frame path.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size >= 5) {
        const std::uint8_t codec_raw = static_cast<std::uint8_t>(data[0] % 3U);
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
