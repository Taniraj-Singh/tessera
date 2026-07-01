#include "tessera/serialize/serializer.hpp"

#include "tessera/catalog/section_catalog.hpp"
#include "tessera/header/container_header.hpp"
#include "tessera/records/record_batch.hpp"
#include "tessera/schema/schema_block.hpp"

namespace tessera::serialize {

core::Result<std::vector<std::uint8_t>> serialize_container(
    const model::Container& container,
    const SerializeOptions& options) {
    model::Container working = container;

    if (options.run_plugins) {
        if (auto status = plugins::Registry::instance().run_hooks(plugins::HookPoint::BeforeSerialize, working); !status) {
            return status.error();
        }
    }

    std::vector<std::vector<std::uint8_t>> section_payloads;
    section_payloads.reserve(working.sections.size());

    for (const model::SectionData& section : working.sections) {
        if (auto schema_bytes = schema::serialize(section.schema); schema_bytes) {
            if (auto record_bytes = records::encode(section.records, section.schema); record_bytes) {
                std::vector<std::uint8_t> combined;
                combined.reserve(schema_bytes->size() + record_bytes->size());
                combined.insert(combined.end(), schema_bytes->begin(), schema_bytes->end());
                combined.insert(combined.end(), record_bytes->begin(), record_bytes->end());
                section_payloads.push_back(std::move(combined));
            } else {
                return record_bytes.error();
            }
        } else {
            return schema_bytes.error();
        }
    }

    catalog::Catalog catalog{};
    catalog.entries.reserve(working.sections.size());
    for (std::size_t index = 0; index < working.sections.size(); ++index) {
        catalog::SectionEntry entry = working.sections[index].entry;
        entry.length = section_payloads[index].size();
        catalog.entries.push_back(entry);
    }

    auto catalog_block = catalog::serialize(catalog);
    if (!catalog_block) {
        return catalog_block.error();
    }

    const std::uint64_t section_base = core::kHeaderSize + catalog_block->size();
    for (std::size_t index = 0; index < catalog.entries.size(); ++index) {
        catalog.entries[index].offset = section_base + [&]() {
            std::uint64_t offset = 0;
            for (std::size_t prior = 0; prior < index; ++prior) {
                offset += section_payloads[prior].size();
            }
            return offset;
        }();
    }

    catalog_block = catalog::serialize(catalog);
    if (!catalog_block) {
        return catalog_block.error();
    }

    header::ContainerHeader header = working.header;
    header.section_count = static_cast<std::uint32_t>(catalog.entries.size());

    auto header_bytes = header::serialize(header);
    if (!header_bytes) {
        return header_bytes.error();
    }

    std::vector<std::uint8_t> output;
    output.reserve(static_cast<std::size_t>(section_base));
    for (const std::vector<std::uint8_t>& payload : section_payloads) {
        output.reserve(output.size() + payload.size());
    }

    output.insert(output.end(), header_bytes->begin(), header_bytes->end());
    output.insert(output.end(), catalog_block->begin(), catalog_block->end());
    for (const std::vector<std::uint8_t>& payload : section_payloads) {
        output.insert(output.end(), payload.begin(), payload.end());
    }

    return output;
}

} // namespace tessera::serialize
