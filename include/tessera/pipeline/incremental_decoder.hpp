#pragma once

#include "tessera/catalog/section_catalog.hpp"
#include "tessera/core/types.hpp"
#include "tessera/header/container_header.hpp"
#include "tessera/memory/byte_buffer.hpp"
#include "tessera/model/container.hpp"
#include "tessera/parse/parser.hpp"

#include <cstddef>
#include <cstdint>

namespace tessera::pipeline {

// Upper bound on a container assembled incrementally. A catalog that implies a
// larger footprint is rejected rather than causing the decoder to wait forever
// for bytes that will never arrive.
inline constexpr std::size_t kMaxIncrementalContainerSize = 256U * 1024U * 1024U;

// Push-model container decoder. Bytes are fed in arbitrary chunks; the decoder
// advances a state machine as each structural boundary becomes available:
//
//   NeedHeader -> NeedCatalogLength -> NeedCatalog -> NeedSections -> Complete
//
// It tracks the catalog block length and the total expected size across feeds,
// and only performs the authoritative full parse once every declared byte is
// present. Any structural error transitions to Failed and is sticky.
class IncrementalDecoder {
public:
    enum class State {
        NeedHeader,
        NeedCatalogLength,
        NeedCatalog,
        NeedSections,
        Complete,
        Failed,
    };

    explicit IncrementalDecoder(parse::ParseOptions options = {}) : options_(options) {}

    // Appends a chunk and advances as far as the accumulated bytes allow.
    // Returns the sticky error once the stream is known to be malformed.
    [[nodiscard]] core::Result<void> feed(core::ByteSpan chunk);

    [[nodiscard]] State state() const noexcept { return state_; }
    [[nodiscard]] bool complete() const noexcept { return state_ == State::Complete; }
    [[nodiscard]] bool failed() const noexcept { return state_ == State::Failed; }
    [[nodiscard]] std::size_t buffered() const noexcept { return buffer_.size(); }

    // Valid only once complete(). Moves the parsed container out.
    [[nodiscard]] core::Result<model::Container> take();

private:
    core::Result<void> advance();
    core::Result<void> fail(core::Error error);

    parse::ParseOptions options_{};
    memory::ByteBuffer buffer_;
    State state_{State::NeedHeader};
    core::Error error_{};

    header::ContainerHeader header_{};
    std::uint32_t catalog_block_length_{0};
    std::uint64_t expected_size_{0};
    model::Container result_{};
};

} // namespace tessera::pipeline
