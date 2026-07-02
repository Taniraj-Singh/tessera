#pragma once

#include "tessera/core/types.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace tessera::pipeline {

struct PipelineOptions {
    bool run_structural{true}; // validate_model on each decoded container
    bool run_semantic{true};   // validate_semantics on each decoded container
};

// Per-entry result of running the archive pipeline.
struct EntryOutcome {
    std::string name;
    bool decoded{false};
    bool valid{true};
    core::ErrorCode error{core::ErrorCode::Ok};
    std::size_t section_count{0};
    std::size_t row_total{0};
};

struct PipelineReport {
    bool opened{false};
    core::ErrorCode open_error{core::ErrorCode::Ok};
    std::size_t entries{0};
    std::size_t decoded{0};
    std::size_t valid{0};
    std::vector<EntryOutcome> outcomes;
};

// Opens an archive and drives every entry through decode + validation, folding
// the results into a single report. This is the top-level orchestration that
// binds the archive, compression, parse, and validation subsystems end to end.
[[nodiscard]] PipelineReport run_archive_pipeline(core::ByteSpan archive, const PipelineOptions& options = {});

} // namespace tessera::pipeline
