#include "tessera/tessera.hpp"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const tessera::core::ByteSpan input{data, size};
    (void)tessera::header::parse(input);

    if (size > tessera::core::kHeaderSize) {
        const tessera::core::ByteSpan catalog{data + tessera::core::kHeaderSize, size - tessera::core::kHeaderSize};
        (void)tessera::catalog::parse(catalog, size);
    }
    return 0;
}
