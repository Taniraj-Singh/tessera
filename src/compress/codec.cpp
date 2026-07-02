#include "tessera/compress/codec.hpp"

#include "tessera/compress/delta_codec.hpp"
#include "tessera/compress/lz77_codec.hpp"
#include "tessera/compress/rle_codec.hpp"
#include "tessera/compress/store_codec.hpp"

namespace tessera::compress {

core::Result<CodecId> parse_codec_id(std::uint8_t raw) {
    switch (raw) {
    case static_cast<std::uint8_t>(CodecId::Store):
    case static_cast<std::uint8_t>(CodecId::Rle):
    case static_cast<std::uint8_t>(CodecId::Lz77):
    case static_cast<std::uint8_t>(CodecId::Delta):
        return static_cast<CodecId>(raw);
    default:
        return core::Error{core::ErrorCode::InvalidEnum, "unknown codec id", 0};
    }
}

const Codec* find_codec(CodecId id) noexcept {
    static const StoreCodec store;
    static const RleCodec rle;
    static const Lz77Codec lz77;
    static const DeltaCodec delta;
    switch (id) {
    case CodecId::Store:
        return &store;
    case CodecId::Rle:
        return &rle;
    case CodecId::Lz77:
        return &lz77;
    case CodecId::Delta:
        return &delta;
    }
    return nullptr;
}

std::string_view codec_name(CodecId id) noexcept {
    if (const Codec* codec = find_codec(id)) {
        return codec->name();
    }
    return "unknown";
}

} // namespace tessera::compress
