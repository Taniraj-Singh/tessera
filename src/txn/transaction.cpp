#include "tessera/txn/transaction.hpp"

#include "tessera/core/constants.hpp"
#include "tessera/validate/semantic_validator.hpp"

namespace tessera::txn {

Transaction::Transaction(model::Container base) : base_(std::move(base)), working_(base_) {}

std::size_t Transaction::find_slot(std::uint16_t id) const {
    for (std::size_t slot = 0; slot < working_.sections.size(); ++slot) {
        if (working_.sections[slot].entry.id == id) {
            return slot;
        }
    }
    return kNotFound;
}

void Transaction::resync_header() {
    working_.catalog.entries.clear();
    working_.catalog.entries.reserve(working_.sections.size());
    for (const model::SectionData& section : working_.sections) {
        working_.catalog.entries.push_back(section.entry);
    }
    working_.header.section_count = static_cast<std::uint32_t>(working_.sections.size());
}

core::Result<void> Transaction::add_section(model::SectionData section) {
    if (section.entry.name.empty() || section.entry.name.size() > core::kMaxSectionNameLength) {
        return core::Error{core::ErrorCode::InvalidLength, "invalid section name", 0};
    }
    if (find_slot(section.entry.id) != kNotFound) {
        return core::Error{core::ErrorCode::DuplicateSectionId, "section id already present", 0};
    }
    working_.sections.push_back(std::move(section));
    resync_header();
    ++pending_ops_;
    return core::Ok();
}

core::Result<void> Transaction::remove_section(std::uint16_t id) {
    const std::size_t slot = find_slot(id);
    if (slot == kNotFound) {
        return core::Error{core::ErrorCode::SectionOutOfBounds, "section id not found", 0};
    }
    working_.sections.erase(working_.sections.begin() + static_cast<std::ptrdiff_t>(slot));
    resync_header();
    ++pending_ops_;
    return core::Ok();
}

core::Result<void> Transaction::rename_section(std::uint16_t id, std::string new_name) {
    if (new_name.empty() || new_name.size() > core::kMaxSectionNameLength) {
        return core::Error{core::ErrorCode::InvalidLength, "invalid section name", 0};
    }
    const std::size_t slot = find_slot(id);
    if (slot == kNotFound) {
        return core::Error{core::ErrorCode::SectionOutOfBounds, "section id not found", 0};
    }
    for (const model::SectionData& section : working_.sections) {
        if (section.entry.id != id && section.entry.name == new_name) {
            return core::Error{core::ErrorCode::DuplicateSectionName, "section name already present", 0};
        }
    }
    working_.sections[slot].entry.name = std::move(new_name);
    resync_header();
    ++pending_ops_;
    return core::Ok();
}

void Transaction::rollback() {
    working_ = base_;
    pending_ops_ = 0;
}

core::Result<model::Container> Transaction::commit() const {
    const validate::ValidationReport report = validate::validate_semantics(working_, {});
    if (!report.valid) {
        const validate::Diagnostic& first = report.diagnostics.front();
        return core::Error{first.code, "transaction commit rejected: " + first.message, first.offset};
    }
    return working_;
}

} // namespace tessera::txn
