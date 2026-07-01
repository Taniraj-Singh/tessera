#include "tessera/validate/validator.hpp"

#include "tessera/parse/parser.hpp"
#include "tessera/serialize/serializer.hpp"

namespace tessera::validate {

ValidationReport validate_bytes(core::ByteSpan input, const ValidationOptions& options) {
    ValidationReport report{};
    parse::ParseOptions parse_options{};
    parse_options.run_plugins = false;

    if (auto container = parse::parse_container(input, parse_options); container) {
        report = validate_model(*container, options);
    } else {
        report.valid = false;
        report.diagnostics.push_back(Diagnostic{container.error().code, container.error().message, container.error().offset});
    }
    return report;
}

ValidationReport validate_model(const model::Container& container, const ValidationOptions& options) {
    ValidationReport report{};

    if (container.header.magic != core::kContainerMagic) {
        report.valid = false;
        report.diagnostics.push_back({core::ErrorCode::InvalidMagic, "invalid magic in model", 0});
    }

    if (container.catalog.entries.size() != container.header.section_count) {
        report.valid = false;
        report.diagnostics.push_back({core::ErrorCode::InvalidLength, "catalog/header section count mismatch", 0});
    }

    for (const model::SectionData& section : container.sections) {
        if (options.strict_schema && section.schema.section_id != section.entry.id) {
            report.valid = false;
            report.diagnostics.push_back({core::ErrorCode::SchemaMismatch, "schema section id mismatch", 0});
        }
        if (section.records.section_id != section.entry.id) {
            report.valid = false;
            report.diagnostics.push_back({core::ErrorCode::SchemaMismatch, "record section id mismatch", 0});
        }
        if (section.records.row_count != section.records.rows.size()) {
            report.valid = false;
            report.diagnostics.push_back({core::ErrorCode::RecordCountMismatch, "record row count mismatch", 0});
        }
    }

    if (options.verify_checksums) {
        if (auto bytes = serialize::serialize_container(container, {}); bytes) {
            if (auto round_trip = parse::parse_container(core::ByteSpan{bytes->data(), bytes->size()}, {}); !round_trip) {
                report.valid = false;
                report.diagnostics.push_back({round_trip.error().code, "round-trip parse failed", round_trip.error().offset});
            }
        } else {
            report.valid = false;
            report.diagnostics.push_back({bytes.error().code, "re-serialization failed", bytes.error().offset});
        }
    }

    if (!report.diagnostics.empty()) {
        report.valid = false;
    }
    return report;
}

} // namespace tessera::validate
