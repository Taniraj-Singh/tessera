#include "tessera/validate/constraint_validator.hpp"

#include "tessera/index/hash_index.hpp"
#include "tessera/index/section_index.hpp"
#include "tessera/index/string_index.hpp"
#include "tessera/reflect/field_ref.hpp"

namespace tessera::validate {

namespace {

void flag(ValidationReport& report, core::ErrorCode code, std::string message, std::size_t offset) {
    report.valid = false;
    report.diagnostics.push_back(Diagnostic{code, std::move(message), offset});
}

// Extracts a signed 64-bit view of any integer field type. Returns false for
// non-integer fields (bool, f64, utf8, bytes).
bool numeric_as_i64(const reflect::FieldRef& ref, std::int64_t& out) {
    if (const std::uint8_t* value = ref.as_u8()) {
        out = static_cast<std::int64_t>(*value);
        return true;
    }
    if (const std::int32_t* value = ref.as_i32()) {
        out = static_cast<std::int64_t>(*value);
        return true;
    }
    if (const std::uint32_t* value = ref.as_u32()) {
        out = static_cast<std::int64_t>(*value);
        return true;
    }
    if (const std::int64_t* value = ref.as_i64()) {
        out = *value;
        return true;
    }
    if (const std::uint64_t* value = ref.as_u64()) {
        out = static_cast<std::int64_t>(*value);
        return true;
    }
    return false;
}

// Resolves the field index within a section's schema, or returns false.
bool field_index_of(const model::SectionData& section, const std::string& field_name, std::size_t& out) {
    for (std::size_t index = 0; index < section.schema.fields.size(); ++index) {
        if (section.schema.fields[index].name == field_name) {
            out = index;
            return true;
        }
    }
    return false;
}

void check_unique(
    ValidationReport& report,
    const model::SectionData& section,
    std::size_t column,
    const Constraint& constraint) {
    index::StringIndex string_seen;
    index::HashIndex numeric_seen;
    std::uint32_t counter = 0;

    for (std::size_t row_index = 0; row_index < section.records.rows.size(); ++row_index) {
        const records::RecordRow& row = section.records.rows[row_index];
        if (column >= row.values.size()) {
            continue;
        }
        const reflect::FieldRef ref(section.schema.fields[column], row.values[column]);

        if (const std::string* text = ref.as_string()) {
            if (string_seen.contains(*text)) {
                flag(report, core::ErrorCode::InvalidLength, "duplicate value for unique field " + constraint.field, row_index);
            } else {
                string_seen.insert(*text, counter++);
            }
            continue;
        }

        std::int64_t numeric = 0;
        if (numeric_as_i64(ref, numeric)) {
            const std::uint64_t key = static_cast<std::uint64_t>(numeric);
            if (numeric_seen.contains(key)) {
                flag(report, core::ErrorCode::InvalidLength, "duplicate value for unique field " + constraint.field, row_index);
            } else {
                numeric_seen.insert(key, counter++);
            }
            continue;
        }
        flag(report, core::ErrorCode::InternalError, "unique constraint on unsupported field type", row_index);
    }
}

void check_field(
    ValidationReport& report,
    const model::SectionData& section,
    std::size_t column,
    const Constraint& constraint) {
    if (constraint.kind == ConstraintKind::Unique) {
        check_unique(report, section, column, constraint);
        return;
    }

    bool have_previous = false;
    std::int64_t previous = 0;

    for (std::size_t row_index = 0; row_index < section.records.rows.size(); ++row_index) {
        const records::RecordRow& row = section.records.rows[row_index];
        if (column >= row.values.size()) {
            continue;
        }
        const reflect::FieldRef ref(section.schema.fields[column], row.values[column]);

        switch (constraint.kind) {
        case ConstraintKind::NotEmpty: {
            const std::string* text = ref.as_string();
            const std::vector<std::uint8_t>* blob = ref.as_bytes();
            if ((text != nullptr && text->empty()) || (blob != nullptr && blob->empty())) {
                flag(report, core::ErrorCode::InvalidLength, "empty value for non-empty field " + constraint.field, row_index);
            }
            break;
        }
        case ConstraintKind::MinValue:
        case ConstraintKind::MaxValue:
        case ConstraintKind::Monotonic: {
            std::int64_t numeric = 0;
            if (!numeric_as_i64(ref, numeric)) {
                flag(report, core::ErrorCode::SchemaMismatch, "numeric constraint on non-numeric field " + constraint.field, row_index);
                break;
            }
            if (constraint.kind == ConstraintKind::MinValue && numeric < constraint.bound) {
                flag(report, core::ErrorCode::InvalidLength, "value below minimum for field " + constraint.field, row_index);
            } else if (constraint.kind == ConstraintKind::MaxValue && numeric > constraint.bound) {
                flag(report, core::ErrorCode::InvalidLength, "value above maximum for field " + constraint.field, row_index);
            } else if (constraint.kind == ConstraintKind::Monotonic) {
                if (have_previous && numeric < previous) {
                    flag(report, core::ErrorCode::InvalidLength, "non-monotonic value for field " + constraint.field, row_index);
                }
                previous = numeric;
                have_previous = true;
            }
            break;
        }
        case ConstraintKind::Unique:
            break; // handled above
        }
    }
}

} // namespace

ValidationReport check_constraints(const model::Container& container, const std::vector<Constraint>& constraints) {
    ValidationReport report{};
    const index::SectionIndex index = index::SectionIndex::build(container);

    for (const Constraint& constraint : constraints) {
        const std::uint32_t* slot = index.find_by_name(constraint.section);
        if (slot == nullptr || *slot >= container.sections.size()) {
            flag(report, core::ErrorCode::SectionOutOfBounds, "constraint names unknown section " + constraint.section, 0);
            continue;
        }
        const model::SectionData& section = container.sections[*slot];

        std::size_t column = 0;
        if (!field_index_of(section, constraint.field, column)) {
            flag(report, core::ErrorCode::SchemaMismatch, "constraint names unknown field " + constraint.field, 0);
            continue;
        }

        check_field(report, section, column, constraint);
    }

    return report;
}

} // namespace tessera::validate
