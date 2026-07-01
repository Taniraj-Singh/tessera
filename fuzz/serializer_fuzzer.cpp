#include "tessera/tessera.hpp"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (auto parsed = tessera::parse::parse_container(tessera::core::ByteSpan{data, size}, {}); parsed) {
        (void)tessera::serialize::serialize_container(*parsed, {});
    }
    return 0;
}
