#include "tessera/pipeline/incremental_encoder.hpp"

#include "tessera/serialize/serializer.hpp"

namespace tessera::pipeline {

void IncrementalEncoder::begin(std::uint16_t version, std::uint64_t created_unix_ms, std::uint16_t flags) {
    container_ = model::Container{};
    container_.header.magic = core::kContainerMagic;
    container_.header.version = version;
    container_.header.flags = flags;
    container_.header.created_unix_ms = created_unix_ms;
    container_.header.section_count = 0;
    started_ = true;
}

core::Result<void> IncrementalEncoder::add_section(model::SectionData section) {
    if (!started_) {
        begin();
    }
    if (section.entry.name.empty() || section.entry.name.size() > core::kMaxSectionNameLength) {
        return core::Error{core::ErrorCode::InvalidLength, "invalid section name", 0};
    }

    container_.catalog.entries.push_back(section.entry);
    container_.sections.push_back(std::move(section));
    container_.header.section_count = static_cast<std::uint32_t>(container_.sections.size());
    return core::Ok();
}

core::Result<std::vector<std::uint8_t>> IncrementalEncoder::finish() {
    return serialize::serialize_container(container_, {});
}

} // namespace tessera::pipeline
