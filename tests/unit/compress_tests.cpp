#include "support/test_framework.hpp"

#include "tessera/compress/frame.hpp"
#include "tessera/compress/lz77_codec.hpp"
#include "tessera/compress/rle_codec.hpp"
#include "tessera/compress/store_codec.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tessera::test {

namespace {

bool round_trips(const compress::Codec& codec, const std::vector<std::uint8_t>& input) {
    auto compressed = codec.compress(core::ByteSpan{input.data(), input.size()});
    if (!compressed) {
        return false;
    }
    auto restored = codec.decompress(core::ByteSpan{compressed->data(), compressed->size()}, input.size());
    if (!restored) {
        return false;
    }
    return *restored == input;
}

std::vector<std::uint8_t> bytes_of(const std::string& text) {
    return std::vector<std::uint8_t>(text.begin(), text.end());
}

} // namespace

int run_compress_tests() {
    int before = g_failures;

    const std::vector<std::vector<std::uint8_t>> corpus = {
        {},
        {0x00},
        bytes_of("aaaaaaaaaaaaaaaaaaaaaaaa"),
        bytes_of("abababababababababababab"),
        bytes_of("the quick brown fox jumps over the quick brown fox"),
        std::vector<std::uint8_t>(1000, 0x5A),
    };

    compress::StoreCodec store;
    compress::RleCodec rle;
    compress::Lz77Codec lz77;

    for (const auto& sample : corpus) {
        TESSERA_EXPECT_TRUE(round_trips(store, sample));
        TESSERA_EXPECT_TRUE(round_trips(rle, sample));
        TESSERA_EXPECT_TRUE(round_trips(lz77, sample));
    }

    {
        // Highly repetitive data should actually shrink under RLE and LZ77.
        std::vector<std::uint8_t> repetitive(4096, 0x11);
        auto rle_out = rle.compress(core::ByteSpan{repetitive.data(), repetitive.size()});
        auto lz_out = lz77.compress(core::ByteSpan{repetitive.data(), repetitive.size()});
        TESSERA_EXPECT_TRUE(rle_out.ok() && rle_out->size() < repetitive.size());
        TESSERA_EXPECT_TRUE(lz_out.ok() && lz_out->size() < repetitive.size());
    }

    {
        // Frame round-trip across all codecs, including the store fallback path.
        std::vector<std::uint8_t> payload = bytes_of("frame payload frame payload frame payload");
        for (compress::CodecId id : {compress::CodecId::Store, compress::CodecId::Rle, compress::CodecId::Lz77}) {
            auto frame = compress::compress_frame(core::ByteSpan{payload.data(), payload.size()}, id);
            TESSERA_EXPECT_TRUE(frame.ok());
            auto restored = compress::decompress_frame(core::ByteSpan{frame->data(), frame->size()});
            TESSERA_EXPECT_TRUE(restored.ok());
            TESSERA_EXPECT_TRUE(*restored == payload);
        }
    }

    {
        // A corrupted frame magic must be rejected, not misparsed.
        std::vector<std::uint8_t> payload = bytes_of("data");
        auto frame = compress::compress_frame(core::ByteSpan{payload.data(), payload.size()}, compress::CodecId::Store);
        TESSERA_EXPECT_TRUE(frame.ok());
        (*frame)[0] ^= 0xFFU;
        auto restored = compress::decompress_frame(core::ByteSpan{frame->data(), frame->size()});
        TESSERA_EXPECT_TRUE(!restored.ok());
    }

    {
        // A declared size larger than the payload should return an error.
        std::vector<std::uint8_t> tiny = {0x00};
        auto restored = store.decompress(core::ByteSpan{tiny.data(), tiny.size()}, 64);
        TESSERA_EXPECT_TRUE(!restored.ok());
    }

    return g_failures - before;
}

} // namespace tessera::test
