#pragma once

#include "tessera/core/types.hpp"

#include <functional>
#include <utility>
#include <vector>

namespace tessera::pipeline {

// A byte-to-byte transform stage. Decompression, canonicalization, and format
// conversion all fit this shape, so they can be composed into a chain without
// each caller re-threading error handling.
using ByteTransform = std::function<core::Result<std::vector<std::uint8_t>>(core::ByteSpan)>;

// Sequential composition: run `first`, then `second` on its output. A failure in
// either short-circuits.
[[nodiscard]] inline ByteTransform compose(ByteTransform first, ByteTransform second) {
    return [first = std::move(first), second = std::move(second)](
               core::ByteSpan input) -> core::Result<std::vector<std::uint8_t>> {
        auto intermediate = first(input);
        if (!intermediate) {
            return intermediate.error();
        }
        return second(core::ByteSpan{intermediate->data(), intermediate->size()});
    };
}

// Runs a chain of transforms, feeding each stage's output into the next.
[[nodiscard]] inline core::Result<std::vector<std::uint8_t>> run_chain(
    const std::vector<ByteTransform>& stages,
    core::ByteSpan input) {
    std::vector<std::uint8_t> current(input.begin(), input.end());
    for (const ByteTransform& stage : stages) {
        auto next = stage(core::ByteSpan{current.data(), current.size()});
        if (!next) {
            return next.error();
        }
        current = std::move(*next);
    }
    return current;
}

} // namespace tessera::pipeline
