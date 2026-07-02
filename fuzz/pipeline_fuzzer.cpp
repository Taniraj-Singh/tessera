#include "tessera/core/types.hpp"
#include "tessera/pipeline/incremental_decoder.hpp"
#include "tessera/pipeline/pipeline.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

// Exercises the stateful pipeline surfaces. Path A feeds the input to the
// incremental container decoder in small, input-derived chunks so the
// cross-chunk state machine is driven through arbitrary framing. Path B runs the
// full archive pipeline (archive -> frame -> parse -> validate) over the buffer.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    {
        tessera::pipeline::IncrementalDecoder decoder;
        const std::size_t chunk = size != 0 ? static_cast<std::size_t>(data[0] % 7U) + 1U : 1U;
        std::size_t offset = 0;
        while (offset < size) {
            const std::size_t take = std::min(chunk, size - offset);
            if (!decoder.feed(tessera::core::ByteSpan{data + offset, take})) {
                break;
            }
            offset += take;
        }
        (void)decoder.take();
    }

    (void)tessera::pipeline::run_archive_pipeline(tessera::core::ByteSpan{data, size});
    return 0;
}
