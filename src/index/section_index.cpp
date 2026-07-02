#include "tessera/index/section_index.hpp"

namespace tessera::index {

SectionIndex SectionIndex::build(const model::Container& container) {
    SectionIndex index{};
    index.by_id_ = HashIndex(container.sections.size());
    index.by_name_ = StringIndex(container.sections.size());

    for (std::size_t slot = 0; slot < container.sections.size(); ++slot) {
        const model::SectionData& section = container.sections[slot];
        index.by_id_.insert(section.entry.id, static_cast<std::uint32_t>(slot));
        index.by_name_.insert(section.entry.name, static_cast<std::uint32_t>(slot));
    }
    index.count_ = container.sections.size();
    return index;
}

const std::uint32_t* SectionIndex::find_by_id(std::uint16_t id) const {
    return by_id_.find(id);
}

const std::uint32_t* SectionIndex::find_by_name(std::string_view name) const {
    return by_name_.find(name);
}

} // namespace tessera::index
