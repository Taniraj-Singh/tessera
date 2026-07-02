#include "tessera/diff/section_diff.hpp"

namespace tessera::diff {

namespace {

bool schemas_differ(const schema::SchemaBlock& lhs, const schema::SchemaBlock& rhs) {
    if (lhs.fields.size() != rhs.fields.size()) {
        return true;
    }
    for (std::size_t index = 0; index < lhs.fields.size(); ++index) {
        const schema::FieldDescriptor& a = lhs.fields[index];
        const schema::FieldDescriptor& b = rhs.fields[index];
        if (a.name != b.name || a.type != b.type || a.max_length != b.max_length) {
            return true;
        }
    }
    return false;
}

const model::SectionData* find_section(const model::Container& container, std::uint16_t id) {
    for (const model::SectionData& section : container.sections) {
        if (section.entry.id == id) {
            return &section;
        }
    }
    return nullptr;
}

} // namespace

SectionDiff diff_sections(const model::SectionData& base, const model::SectionData& updated) {
    SectionDiff result{};
    result.schema_changed = schemas_differ(base.schema, updated.schema);

    const std::size_t base_rows = base.records.rows.size();
    const std::size_t updated_rows = updated.records.rows.size();
    const std::size_t shared = base_rows < updated_rows ? base_rows : updated_rows;

    for (std::size_t index = 0; index < shared; ++index) {
        if (base.records.rows[index].values == updated.records.rows[index].values) {
            ++result.unchanged;
        } else {
            ++result.modified;
            result.modified_rows.push_back(index);
        }
    }
    if (updated_rows > base_rows) {
        result.added = updated_rows - base_rows;
    } else if (base_rows > updated_rows) {
        result.removed = base_rows - updated_rows;
    }
    return result;
}

ContainerDiff diff_containers(const model::Container& base, const model::Container& updated) {
    ContainerDiff result{};

    for (const model::SectionData& section : base.sections) {
        const model::SectionData* other = find_section(updated, section.entry.id);
        if (other == nullptr) {
            ++result.sections_removed;
            continue;
        }
        const SectionDiff section_diff = diff_sections(section, *other);
        if (!section_diff.identical()) {
            ++result.sections_changed;
        }
        result.total_rows_modified += section_diff.modified;
    }

    for (const model::SectionData& section : updated.sections) {
        if (find_section(base, section.entry.id) == nullptr) {
            ++result.sections_added;
        }
    }
    return result;
}

} // namespace tessera::diff
