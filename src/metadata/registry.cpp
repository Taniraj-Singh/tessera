#include "tessera/metadata/registry.hpp"

namespace tessera::metadata {

MetadataRegistry MetadataRegistry::build(const model::Container& container) {
    MetadataRegistry registry{};
    registry.descriptors_.reserve(container.sections.size());

    for (const model::SectionData& section : container.sections) {
        SectionDescriptor descriptor{};
        descriptor.id = section.entry.id;
        descriptor.name = section.entry.name;
        descriptor.flags = section.entry.flags;
        descriptor.row_count = section.records.row_count;
        descriptor.fields = section.schema.fields;
        registry.descriptors_.push_back(std::move(descriptor));
    }

    // The section index maps id/name to the section's slot, which is the same
    // slot as in descriptors_ because both are built in container order.
    registry.index_ = index::SectionIndex::build(container);
    return registry;
}

const SectionDescriptor* MetadataRegistry::by_id(std::uint16_t id) const {
    const std::uint32_t* slot = index_.find_by_id(id);
    if (slot == nullptr || *slot >= descriptors_.size()) {
        return nullptr;
    }
    return &descriptors_[*slot];
}

const SectionDescriptor* MetadataRegistry::by_name(std::string_view name) const {
    const std::uint32_t* slot = index_.find_by_name(name);
    if (slot == nullptr || *slot >= descriptors_.size()) {
        return nullptr;
    }
    return &descriptors_[*slot];
}

std::size_t MetadataRegistry::total_field_count() const noexcept {
    std::size_t total = 0;
    for (const SectionDescriptor& descriptor : descriptors_) {
        total += descriptor.fields.size();
    }
    return total;
}

} // namespace tessera::metadata
