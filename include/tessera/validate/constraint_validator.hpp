#pragma once

#include "tessera/model/container.hpp"
#include "tessera/validate/validator.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tessera::validate {

// Declarative, out-of-band constraints. They are not part of the TBC format;
// callers supply a rule set that a section's decoded rows must satisfy, which
// lets policy live outside the binary layout.
enum class ConstraintKind {
    NotEmpty,  // aggregate field must be non-empty in every row
    Unique,    // field value must be distinct across rows
    MinValue,  // numeric field >= bound
    MaxValue,  // numeric field <= bound
    Monotonic, // numeric field non-decreasing across rows
};

struct Constraint {
    std::string section; // section name the rule applies to
    std::string field;   // field name within that section
    ConstraintKind kind{ConstraintKind::NotEmpty};
    std::int64_t bound{0}; // used by MinValue / MaxValue
};

// Evaluates every constraint against the matching section/field, accumulating a
// diagnostic per violation. A constraint that names a missing section or field
// is itself a violation.
[[nodiscard]] ValidationReport check_constraints(
    const model::Container& container,
    const std::vector<Constraint>& constraints);

} // namespace tessera::validate
