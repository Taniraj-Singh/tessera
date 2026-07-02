#pragma once

#include "tessera/core/types.hpp"
#include "tessera/validate/validator.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tessera::diag {

enum class Severity : std::uint8_t {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
};

[[nodiscard]] std::string_view severity_name(Severity severity) noexcept;

struct Record {
    Severity severity{Severity::Info};
    std::string category;
    std::string message;
    std::size_t offset{0};
};

// Structured diagnostic collector. Subsystems emit categorized, severity-tagged
// records into a sink; callers query counts, test for errors, or render a
// report. It is the common destination for validation results, recovery notes,
// and pipeline warnings.
class DiagnosticSink {
public:
    void emit(Severity severity, std::string category, std::string message, std::size_t offset = 0);

    [[nodiscard]] const std::vector<Record>& records() const noexcept { return records_; }
    [[nodiscard]] std::size_t count(Severity severity) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return records_.size(); }
    [[nodiscard]] bool has_errors() const noexcept;
    [[nodiscard]] bool empty() const noexcept { return records_.empty(); }

    void clear() noexcept { records_.clear(); }

    // Renders one line per record: "SEVERITY [category] message @offset".
    [[nodiscard]] std::string format() const;

private:
    std::vector<Record> records_;
};

// Folds a validation report's diagnostics into a sink under the given category,
// mapping every validation failure to Severity::Error.
void ingest(DiagnosticSink& sink, const validate::ValidationReport& report, std::string category);

} // namespace tessera::diag
