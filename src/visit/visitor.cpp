#include "tessera/visit/visitor.hpp"

namespace tessera::visit {

void traverse(const model::Container& container, Visitor& visitor) {
    visitor.enter_container(container);
    for (std::size_t index = 0; index < container.sections.size(); ++index) {
        const model::SectionData& section = container.sections[index];
        visitor.enter_section(section, index);

        for (std::size_t field_index = 0; field_index < section.schema.fields.size(); ++field_index) {
            visitor.visit_field(section.schema.fields[field_index], field_index);
        }
        for (std::size_t row_index = 0; row_index < section.records.rows.size(); ++row_index) {
            visitor.visit_row(section.records.rows[row_index], row_index);
        }

        visitor.leave_section(section, index);
    }
    visitor.leave_container(container);
}

} // namespace tessera::visit
