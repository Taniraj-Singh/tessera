#pragma once

#include "tessera/model/container.hpp"
#include "tessera/records/record_batch.hpp"
#include "tessera/schema/schema_block.hpp"

#include <cstddef>

namespace tessera::visit {

// Visitor over the container model. Subclasses override only the hooks they care
// about; the default implementations are no-ops. enter/leave pairs bracket each
// section so stateful visitors (statistics, exporters) can scope per-section
// context.
class Visitor {
public:
    virtual ~Visitor() = default;

    virtual void enter_container(const model::Container& container) { (void)container; }
    virtual void enter_section(const model::SectionData& section, std::size_t index) {
        (void)section;
        (void)index;
    }
    virtual void visit_field(const schema::FieldDescriptor& field, std::size_t index) {
        (void)field;
        (void)index;
    }
    virtual void visit_row(const records::RecordRow& row, std::size_t index) {
        (void)row;
        (void)index;
    }
    virtual void leave_section(const model::SectionData& section, std::size_t index) {
        (void)section;
        (void)index;
    }
    virtual void leave_container(const model::Container& container) { (void)container; }
};

// Walks the container, dispatching to the visitor in structural order:
// container → (section → fields → rows → )* → container.
void traverse(const model::Container& container, Visitor& visitor);

} // namespace tessera::visit
