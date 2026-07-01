#pragma once

#include "tessera/core/types.hpp"
#include "tessera/model/container.hpp"

#include <string>
#include <vector>

namespace tessera::validate {

struct Diagnostic {
    core::ErrorCode code{core::ErrorCode::Ok};
    std::string message;
    std::size_t offset{0};
};

struct ValidationReport {
    bool valid{true};
    std::vector<Diagnostic> diagnostics;
};

struct ValidationOptions {
    bool strict_schema{true};
    bool verify_checksums{true};
};

[[nodiscard]] ValidationReport validate_bytes(core::ByteSpan input, const ValidationOptions& options = {});
[[nodiscard]] ValidationReport validate_model(const model::Container& container, const ValidationOptions& options = {});

} // namespace tessera::validate
