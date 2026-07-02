#pragma once

#include "tessera/model/container.hpp"
#include "tessera/validate/validator.hpp"

namespace tessera::validate {

struct SemanticOptions {
    bool require_type_match{true};  // each value's variant must match its field type
    bool enforce_max_length{true};  // Utf8/Bytes must honor their declared cap
    bool enforce_unique_ids{true};  // section ids and names must be unique
};

// Deep, value-level validation of an in-memory container: field/value type
// agreement, string/blob length caps, UTF-8 well-formedness, row/field counts,
// and section identity uniqueness. Unlike validate_model this inspects every
// decoded value, so it catches defects in programmatically built containers
// before they are serialized.
[[nodiscard]] ValidationReport validate_semantics(
    const model::Container& container,
    const SemanticOptions& options = {});

} // namespace tessera::validate
