#include "tessera/tessera.hpp"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    (void)tessera::validate::validate_bytes(tessera::core::ByteSpan{data, size}, {});
    return 0;
}
