#include "tessera/archive/archive_reader.hpp"
#include "tessera/core/types.hpp"

#include <cstddef>
#include <cstdint>

// Drives the full archive path: header + directory validation, then per-entry
// frame decompression and container parsing. This reaches the deepest logic in
// the SDK — archive directory -> compression frame -> codec -> container parse
// -> catalog -> schema -> record decode — from a single byte buffer.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    auto reader = tessera::archive::ArchiveReader::open(tessera::core::ByteSpan{data, size});
    if (!reader) {
        return 0;
    }
    for (std::size_t index = 0; index < reader->size(); ++index) {
        (void)reader->parse_entry(index);
    }
    return 0;
}
