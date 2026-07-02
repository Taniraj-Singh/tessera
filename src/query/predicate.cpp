#include "tessera/query/predicate.hpp"

#include "tessera/reflect/field_ref.hpp"

#include <cstdint>

namespace tessera::query {

namespace {

template <typename T>
bool compare_ordered(CompareOp op, T lhs, T rhs) {
    switch (op) {
    case CompareOp::Eq:
        return lhs == rhs;
    case CompareOp::Ne:
        return lhs != rhs;
    case CompareOp::Lt:
        return lhs < rhs;
    case CompareOp::Le:
        return lhs <= rhs;
    case CompareOp::Gt:
        return lhs > rhs;
    case CompareOp::Ge:
        return lhs >= rhs;
    }
    return false;
}

bool numeric_value(const reflect::FieldRef& ref, double& out) {
    if (const bool* value = ref.as_bool()) {
        out = *value ? 1.0 : 0.0;
        return true;
    }
    if (const std::uint8_t* value = ref.as_u8()) {
        out = static_cast<double>(*value);
        return true;
    }
    if (const std::int32_t* value = ref.as_i32()) {
        out = static_cast<double>(*value);
        return true;
    }
    if (const std::uint32_t* value = ref.as_u32()) {
        out = static_cast<double>(*value);
        return true;
    }
    if (const std::int64_t* value = ref.as_i64()) {
        out = static_cast<double>(*value);
        return true;
    }
    if (const std::uint64_t* value = ref.as_u64()) {
        out = static_cast<double>(*value);
        return true;
    }
    if (const double* value = ref.as_f64()) {
        out = *value;
        return true;
    }
    return false;
}

bool find_field(const model::SectionData& section, const std::string& name, std::size_t& out) {
    for (std::size_t index = 0; index < section.schema.fields.size(); ++index) {
        if (section.schema.fields[index].name == name) {
            out = index;
            return true;
        }
    }
    return false;
}

bool row_matches(const model::SectionData& section, const records::RecordRow& row, const std::vector<Predicate>& predicates) {
    for (const Predicate& predicate : predicates) {
        std::size_t column = 0;
        if (!find_field(section, predicate.field, column) || column >= row.values.size()) {
            return false;
        }
        const reflect::FieldRef ref(section.schema.fields[column], row.values[column]);
        if (predicate.is_text) {
            const std::string* text = ref.as_string();
            if (text == nullptr || !compare_ordered(predicate.op, *text, predicate.text)) {
                return false;
            }
        } else {
            double value = 0.0;
            if (!numeric_value(ref, value) || !compare_ordered(predicate.op, value, predicate.number)) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

std::vector<std::size_t> select_rows(const model::SectionData& section, const std::vector<Predicate>& predicates) {
    std::vector<std::size_t> matches;
    for (std::size_t index = 0; index < section.records.rows.size(); ++index) {
        if (row_matches(section, section.records.rows[index], predicates)) {
            matches.push_back(index);
        }
    }
    return matches;
}

std::size_t count_matching(const model::SectionData& section, const std::vector<Predicate>& predicates) {
    std::size_t total = 0;
    for (const records::RecordRow& row : section.records.rows) {
        if (row_matches(section, row, predicates)) {
            ++total;
        }
    }
    return total;
}

model::SectionData filter_section(const model::SectionData& section, const std::vector<Predicate>& predicates) {
    model::SectionData filtered{};
    filtered.entry = section.entry;
    filtered.schema = section.schema;
    filtered.records.section_id = section.records.section_id;
    for (const records::RecordRow& row : section.records.rows) {
        if (row_matches(section, row, predicates)) {
            filtered.records.rows.push_back(row);
        }
    }
    filtered.records.row_count = static_cast<std::uint32_t>(filtered.records.rows.size());
    return filtered;
}

} // namespace tessera::query
