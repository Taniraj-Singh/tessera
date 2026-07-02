#pragma once

#include "tessera/model/container.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tessera::query {

enum class CompareOp {
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
};

// A single field comparison. Numeric fields compare against `number`; string
// fields compare lexicographically against `text`. `is_text` selects which
// operand is meaningful.
struct Predicate {
    std::string field;
    CompareOp op{CompareOp::Eq};
    bool is_text{false};
    double number{0.0};
    std::string text;

    static Predicate numeric(std::string field, CompareOp op, double value) {
        return Predicate{std::move(field), op, false, value, {}};
    }
    static Predicate string_value(std::string field, CompareOp op, std::string value) {
        return Predicate{std::move(field), op, true, 0.0, std::move(value)};
    }
};

// Returns the indices of the rows in `section` that satisfy every predicate
// (logical AND). A predicate naming an absent field matches nothing.
[[nodiscard]] std::vector<std::size_t> select_rows(
    const model::SectionData& section,
    const std::vector<Predicate>& predicates);

[[nodiscard]] std::size_t count_matching(
    const model::SectionData& section,
    const std::vector<Predicate>& predicates);

// Produces a copy of `section` containing only the matching rows, with the row
// count kept consistent.
[[nodiscard]] model::SectionData filter_section(
    const model::SectionData& section,
    const std::vector<Predicate>& predicates);

} // namespace tessera::query
