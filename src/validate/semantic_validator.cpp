#include "tessera/validate/semantic_validator.hpp"

#include "tessera/core/types.hpp"
#include "tessera/reflect/field_ref.hpp"
#include "tessera/schema/schema_compiler.hpp"

#include "tessera/index/hash_index.hpp"
#include "tessera/index/string_index.hpp"

namespace tessera::validate {

namespace {

void flag(ValidationReport& report, core::ErrorCode code, std::string message, std::size_t offset) {
    report.valid = false;
    report.diagnostics.push_back(Diagnostic{code, std::move(message), offset});
}

void validate_row(
    ValidationReport& report,
    const schema::CompiledSchema& compiled,
    const records::RecordRow& row,
    std::size_t row_index,
    const SemanticOptions& options) {
    if (row.values.size() != compiled.field_count()) {
        flag(report, core::ErrorCode::SchemaMismatch, "row field count mismatch", row_index);
        return;
    }

    for (std::size_t column = 0; column < compiled.fields().size(); ++column) {
        const schema::CompiledField& field = compiled.fields()[column];
        const reflect::FieldRef ref(field.descriptor, row.values[column]);

        if (options.require_type_match && !ref.matches_type()) {
            flag(report, core::ErrorCode::SchemaMismatch, "value type does not match field " + field.descriptor.name, row_index);
            continue;
        }

        if (!options.enforce_max_length) {
            continue;
        }
        if (const std::string* text = ref.as_string(); text != nullptr) {
            if (field.descriptor.max_length != 0 && text->size() > field.descriptor.max_length) {
                flag(report, core::ErrorCode::InvalidLength, "utf8 field exceeds max length: " + field.descriptor.name, row_index);
            }
            if (!core::is_valid_utf8(*text)) {
                flag(report, core::ErrorCode::InvalidUtf8, "invalid utf8 in field " + field.descriptor.name, row_index);
            }
        } else if (const std::vector<std::uint8_t>* blob = ref.as_bytes(); blob != nullptr) {
            if (field.descriptor.max_length != 0 && blob->size() > field.descriptor.max_length) {
                flag(report, core::ErrorCode::InvalidLength, "bytes field exceeds max length: " + field.descriptor.name, row_index);
            }
        }
    }
}

} // namespace

ValidationReport validate_semantics(const model::Container& container, const SemanticOptions& options) {
    ValidationReport report{};

    index::HashIndex seen_ids;
    index::StringIndex seen_names;

    for (std::size_t slot = 0; slot < container.sections.size(); ++slot) {
        const model::SectionData& section = container.sections[slot];

        if (options.enforce_unique_ids) {
            if (seen_ids.contains(section.entry.id)) {
                flag(report, core::ErrorCode::DuplicateSectionId, "duplicate section id", slot);
            } else {
                seen_ids.insert(section.entry.id, static_cast<std::uint32_t>(slot));
            }
            if (seen_names.contains(section.entry.name)) {
                flag(report, core::ErrorCode::DuplicateSectionName, "duplicate section name", slot);
            } else {
                seen_names.insert(section.entry.name, static_cast<std::uint32_t>(slot));
            }
        }

        if (section.schema.section_id != section.entry.id) {
            flag(report, core::ErrorCode::SchemaMismatch, "schema/section id mismatch", slot);
        }
        if (section.records.section_id != section.entry.id) {
            flag(report, core::ErrorCode::SchemaMismatch, "record/section id mismatch", slot);
        }
        if (section.records.row_count != section.records.rows.size()) {
            flag(report, core::ErrorCode::RecordCountMismatch, "declared row count mismatch", slot);
        }

        auto compiled = schema::compile(section.schema);
        if (!compiled) {
            flag(report, compiled.error().code, compiled.error().message, slot);
            continue;
        }

        for (std::size_t row_index = 0; row_index < section.records.rows.size(); ++row_index) {
            validate_row(report, *compiled, section.records.rows[row_index], row_index, options);
        }
    }

    return report;
}

} // namespace tessera::validate
