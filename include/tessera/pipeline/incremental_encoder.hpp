#pragma once

#include "tessera/core/constants.hpp"
#include "tessera/core/types.hpp"
#include "tessera/model/container.hpp"

#include <cstdint>
#include <vector>

namespace tessera::pipeline {

// Builder that assembles a container section by section, then serializes it.
// The mirror of IncrementalDecoder: callers stream sections in and receive the
// canonical byte layout at finish(), with the header section count kept in sync.
class IncrementalEncoder {
public:
    IncrementalEncoder() = default;

    void begin(std::uint16_t version = core::kContainerVersion, std::uint64_t created_unix_ms = 0, std::uint16_t flags = 0);

    // Appends a section. Rejects an empty section name so the resulting catalog
    // is well-formed.
    [[nodiscard]] core::Result<void> add_section(model::SectionData section);

    [[nodiscard]] core::Result<std::vector<std::uint8_t>> finish();

    [[nodiscard]] std::size_t section_count() const noexcept { return container_.sections.size(); }

private:
    model::Container container_{};
    bool started_{false};
};

} // namespace tessera::pipeline
