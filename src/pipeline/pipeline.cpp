#include "tessera/pipeline/pipeline.hpp"

#include "tessera/archive/archive_reader.hpp"
#include "tessera/validate/semantic_validator.hpp"
#include "tessera/validate/validator.hpp"

namespace tessera::pipeline {

namespace {

std::size_t count_rows(const model::Container& container) {
    std::size_t total = 0;
    for (const model::SectionData& section : container.sections) {
        total += section.records.rows.size();
    }
    return total;
}

} // namespace

PipelineReport run_archive_pipeline(core::ByteSpan archive, const PipelineOptions& options) {
    PipelineReport report{};

    auto reader = archive::ArchiveReader::open(archive);
    if (!reader) {
        report.opened = false;
        report.open_error = reader.error().code;
        return report;
    }
    report.opened = true;
    report.entries = reader->size();
    report.outcomes.reserve(reader->size());

    for (std::size_t index = 0; index < reader->size(); ++index) {
        EntryOutcome outcome{};
        outcome.name = reader->entries()[index].name;

        auto container = reader->parse_entry(index);
        if (!container) {
            outcome.decoded = false;
            outcome.valid = false;
            outcome.error = container.error().code;
            report.outcomes.push_back(std::move(outcome));
            continue;
        }

        outcome.decoded = true;
        outcome.section_count = container->sections.size();
        outcome.row_total = count_rows(*container);
        ++report.decoded;

        bool valid = true;
        if (options.run_structural) {
            const validate::ValidationReport structural = validate::validate_model(*container, {});
            valid = valid && structural.valid;
        }
        if (options.run_semantic) {
            const validate::ValidationReport semantic = validate::validate_semantics(*container, {});
            valid = valid && semantic.valid;
        }
        outcome.valid = valid;
        if (valid) {
            ++report.valid;
        }

        report.outcomes.push_back(std::move(outcome));
    }

    return report;
}

} // namespace tessera::pipeline
