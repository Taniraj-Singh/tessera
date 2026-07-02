#include "tessera/compress/store_codec.hpp"

namespace tessera::compress {

core::Result<std::vector<std::uint8_t>> StoreCodec::compress(core::ByteSpan input) const {
    return std::vector<std::uint8_t>(input.begin(), input.end());
}

core::Result<std::vector<std::uint8_t>> StoreCodec::decompress(
    core::ByteSpan input,
    std::size_t expected_size) const {
    if (input.size != expected_size) {
        return core::Error{core::ErrorCode::InvalidLength, "store payload size mismatch", 0};
    }
    return std::vector<std::uint8_t>(input.begin(), input.end());
}

} // namespace tessera::compress
