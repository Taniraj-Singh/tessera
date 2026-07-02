#include "tessera/tessera.hpp"

#include "tessera/compress/lz77_codec.hpp"
#include "tessera/compress/rle_codec.hpp"
#include "tessera/compress/store_codec.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

void report_codec(const tessera::compress::Codec& codec, const std::vector<std::uint8_t>& input) {
    auto compressed = codec.compress(tessera::core::ByteSpan{input.data(), input.size()});
    if (!compressed) {
        std::cout << "  " << codec.name() << ": compress failed\n";
        return;
    }
    auto restored = codec.decompress(
        tessera::core::ByteSpan{compressed->data(), compressed->size()}, input.size());
    const bool ok = restored && *restored == input;
    std::cout << "  " << codec.name() << ": " << input.size() << " -> " << compressed->size()
              << " bytes, roundtrip " << (ok ? "ok" : "FAILED") << '\n';
}

} // namespace

int main() {
    // Repetitive payload that both RLE and LZ77 can exploit.
    std::vector<std::uint8_t> data;
    const std::string phrase = "tessera-binary-container-";
    for (int repeat = 0; repeat < 64; ++repeat) {
        for (char character : phrase) {
            data.push_back(static_cast<std::uint8_t>(character));
        }
    }
    std::cout << "Payload: " << data.size() << " bytes\n";

    const tessera::compress::StoreCodec store;
    const tessera::compress::RleCodec rle;
    const tessera::compress::Lz77Codec lz77;
    report_codec(store, data);
    report_codec(rle, data);
    report_codec(lz77, data);

    // Frame round-trip with the store-fallback logic exercised on random-ish data.
    auto frame = tessera::compress::compress_frame(
        tessera::core::ByteSpan{data.data(), data.size()}, tessera::compress::CodecId::Lz77);
    if (frame) {
        std::cout << "Frame: " << frame->size() << " bytes\n";
        auto restored = tessera::compress::decompress_frame(
            tessera::core::ByteSpan{frame->data(), frame->size()});
        std::cout << "Frame roundtrip: " << ((restored && *restored == data) ? "ok" : "FAILED") << '\n';
    }
    return 0;
}
