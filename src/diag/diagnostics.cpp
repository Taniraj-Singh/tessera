#include "tessera/diag/diagnostics.hpp"

#include <sstream>

namespace tessera::diag {

std::string_view severity_name(Severity severity) noexcept {
    switch (severity) {
    case Severity::Trace:
        return "TRACE";
    case Severity::Debug:
        return "DEBUG";
    case Severity::Info:
        return "INFO";
    case Severity::Warning:
        return "WARNING";
    case Severity::Error:
        return "ERROR";
    case Severity::Fatal:
        return "FATAL";
    }
    return "UNKNOWN";
}

void DiagnosticSink::emit(Severity severity, std::string category, std::string message, std::size_t offset) {
    records_.push_back(Record{severity, std::move(category), std::move(message), offset});
}

std::size_t DiagnosticSink::count(Severity severity) const noexcept {
    std::size_t total = 0;
    for (const Record& record : records_) {
        if (record.severity == severity) {
            ++total;
        }
    }
    return total;
}

bool DiagnosticSink::has_errors() const noexcept {
    for (const Record& record : records_) {
        if (record.severity == Severity::Error || record.severity == Severity::Fatal) {
            return true;
        }
    }
    return false;
}

std::string DiagnosticSink::format() const {
    std::ostringstream out;
    for (const Record& record : records_) {
        out << severity_name(record.severity) << " [" << record.category << "] " << record.message
            << " @" << record.offset << '\n';
    }
    return out.str();
}

void ingest(DiagnosticSink& sink, const validate::ValidationReport& report, std::string category) {
    for (const validate::Diagnostic& diagnostic : report.diagnostics) {
        sink.emit(Severity::Error, category, diagnostic.message, diagnostic.offset);
    }
}

} // namespace tessera::diag
