#include "tessera/archive/archive_reader.hpp"
#include "tessera/core/types.hpp"

#include <cstddef>
#include <cstdint>

// Opens a byte buffer as an archive and parses each entry, exercising the
// archive reader, the compression frame layer, and the container parser.
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
