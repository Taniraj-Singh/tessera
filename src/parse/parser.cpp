#include "tessera/parse/parser.hpp"

#include "tessera/catalog/section_catalog.hpp"
#include "tessera/header/container_header.hpp"
#include "tessera/records/record_batch.hpp"
#include "tessera/schema/schema_block.hpp"

namespace tessera::parse {

namespace {

core::Result<std::vector<std::uint8_t>> read_section_bytes(core::ByteSpan input, const catalog::SectionEntry& entry) {
    if (entry.offset + entry.length > input.size) {
        return core::Error{core::ErrorCode::SectionOutOfBounds, "section bytes unavailable", static_cast<std::size_t>(entry.offset)};
    }
    return std::vector<std::uint8_t>(input.data + entry.offset, input.data + entry.offset + entry.length);
}

} // namespace

core::Result<model::Container> parse_container(core::ByteSpan input, const ParseOptions& options) {
    model::Container container{};

    if (auto header = header::parse(input); header) {
        container.header = *header;
    } else {
        return header.error();
    }

    version::VersionRange range = options.version_range;
    if (range.max_supported == 0) {
        range.min_supported = core::kContainerVersion;
        range.max_supported = core::kContainerVersion;
    }
    if (auto negotiated = version::negotiate(container.header.version, range); !negotiated) {
        return negotiated.error();
    }

    if (options.run_plugins) {
        if (auto status = plugins::Registry::instance().run_hooks(plugins::HookPoint::AfterHeader, container); !status) {
            return status.error();
        }
    }

    if (input.size < core::kHeaderSize + 8) {
        return core::Error{core::ErrorCode::TruncatedInput, "missing catalog block", input.size};
    }

    const core::ByteSpan catalog_block{input.data + core::kHeaderSize, input.size - core::kHeaderSize};
    if (auto catalog = catalog::parse(catalog_block, input.size); catalog) {
        container.catalog = std::move(*catalog);
    } else {
        return catalog.error();
    }

    if (container.catalog.entries.size() != container.header.section_count) {
        return core::Error{core::ErrorCode::InvalidLength, "section count mismatch", core::kHeaderSize};
    }

    if (options.run_plugins) {
        if (auto status = plugins::Registry::instance().run_hooks(plugins::HookPoint::AfterCatalog, container); !status) {
            return status.error();
        }
    }

    container.sections.reserve(container.catalog.entries.size());
    for (const catalog::SectionEntry& entry : container.catalog.entries) {
        model::SectionData section{};
        section.entry = entry;

        if (auto bytes = read_section_bytes(input, entry); bytes) {
            const core::ByteSpan section_span{bytes->data(), bytes->size()};
            if (section_span.size < 4) {
                return core::Error{core::ErrorCode::TruncatedInput, "section payload truncated", static_cast<std::size_t>(entry.offset)};
            }

            const std::uint32_t schema_block_length = core::read_le_u32(section_span.data);
            if (schema_block_length > section_span.size) {
                return core::Error{core::ErrorCode::InvalidLength, "schema block length exceeds section", static_cast<std::size_t>(entry.offset)};
            }

            const core::ByteSpan schema_span{section_span.data, schema_block_length};
            if (auto schema = schema::parse(schema_span); schema) {
                section.schema = std::move(*schema);
            } else {
                return schema.error();
            }

            if (options.run_plugins) {
                model::Container interim = container;
                interim.sections.push_back(section);
                if (auto status = plugins::Registry::instance().run_hooks(plugins::HookPoint::AfterSchema, interim); !status) {
                    return status.error();
                }
            }

            if (schema_block_length >= section_span.size) {
                return core::Error{core::ErrorCode::TruncatedInput, "missing record batch in section", static_cast<std::size_t>(entry.offset)};
            }

            const core::ByteSpan records_span{
                section_span.data + schema_block_length,
                section_span.size - schema_block_length,
            };
            if (auto records = records::decode(records_span, section.schema); records) {
                section.records = std::move(*records);
            } else {
                return records.error();
            }
        } else {
            return bytes.error();
        }

        container.sections.push_back(std::move(section));
    }

    if (options.run_plugins) {
        if (auto status = plugins::Registry::instance().run_hooks(plugins::HookPoint::AfterRecords, container); !status) {
            return status.error();
        }
    }

    return container;
}

} // namespace tessera::parse
