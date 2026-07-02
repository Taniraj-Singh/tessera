#pragma once

#include "tessera/index/section_index.hpp"
#include "tessera/model/container.hpp"
#include "tessera/schema/schema_block.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tessera::metadata {

// Denormalized description of one section: identity, flags, its schema fields,
// and the decoded row count. Built once from a container so downstream passes
// (semantic validation, conversion, statistics) share a single introspection
// snapshot instead of re-deriving it.
struct SectionDescriptor {
    std::uint16_t id{0};
    std::string name;
    std::uint16_t flags{0};
    std::uint32_t row_count{0};
    std::vector<schema::FieldDescriptor> fields;
};

class MetadataRegistry {
public:
    MetadataRegistry() = default;

    [[nodiscard]] static MetadataRegistry build(const model::Container& container);

    [[nodiscard]] const SectionDescriptor* by_id(std::uint16_t id) const;
    [[nodiscard]] const SectionDescriptor* by_name(std::string_view name) const;

    [[nodiscard]] const std::vector<SectionDescriptor>& descriptors() const noexcept { return descriptors_; }
    [[nodiscard]] std::size_t size() const noexcept { return descriptors_.size(); }

    // Total number of fields across all sections — a cheap complexity metric
    // used by the statistics and profiler subsystems.
    [[nodiscard]] std::size_t total_field_count() const noexcept;

private:
    std::vector<SectionDescriptor> descriptors_;
    index::SectionIndex index_;
};

} // namespace tessera::metadata
