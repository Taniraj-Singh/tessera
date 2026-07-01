#include "tessera/tessera.hpp"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size < 8) {
        return 0;
    }

    const tessera::core::ByteSpan block{data, size};
    if (auto schema = tessera::schema::parse(block); schema) {
        const tessera::core::ByteSpan records{data, size};
        (void)tessera::records::decode(records, *schema);
    }
    return 0;
}
