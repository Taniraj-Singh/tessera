#pragma once

#include "tessera/core/types.hpp"
#include "tessera/model/container.hpp"
#include "tessera/records/record_batch.hpp"
#include "tessera/schema/schema_block.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace tessera::textio {

// Renders a section as RFC-4180-style CSV: a header row of field names followed
// by one row per record. Cells containing a comma, quote, or newline are quoted
// and embedded quotes are doubled.
[[nodiscard]] std::string section_to_csv(const model::SectionData& section);

// Parses CSV rows (skipping the header line) into records, coercing each cell to
// its field type per the provided schema.
[[nodiscard]] core::Result<std::vector<records::RecordRow>> rows_from_csv(
    std::string_view text,
    const schema::SchemaBlock& schema);

} // namespace tessera::textio
