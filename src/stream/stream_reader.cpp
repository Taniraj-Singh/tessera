#include "tessera/stream/stream_reader.hpp"

#include "tessera/parse/parser.hpp"
#include "tessera/records/record_batch.hpp"
#include "tessera/schema/schema_block.hpp"

namespace tessera::stream {

StreamReader::StreamReader(core::ByteSpan input) : input_(input) {}

core::Result<StreamEvent> StreamReader::read_header() {
    auto parsed = header::parse(input_);
    if (parsed) {
        header_ = *parsed;
        phase_ = Phase::Catalog;
        StreamEvent event{};
        event.kind = StreamEventKind::Header;
        event.header = header_;
        return event;
    }
    return parsed.error();
}

core::Result<StreamEvent> StreamReader::read_catalog() {
    if (input_.size < core::kHeaderSize + 8) {
        return core::Error{core::ErrorCode::TruncatedInput, "missing catalog block", input_.size};
    }
    const core::ByteSpan catalog_block{input_.data + core::kHeaderSize, input_.size - core::kHeaderSize};
    auto parsed = catalog::parse(catalog_block, input_.size);
    if (parsed) {
        catalog_ = std::move(*parsed);
        phase_ = Phase::Sections;
        section_index_ = 0;
        StreamEvent event{};
        event.kind = StreamEventKind::Catalog;
        event.catalog = catalog_;
        return event;
    }
    return parsed.error();
}

core::Result<StreamEvent> StreamReader::read_section() {
    if (section_index_ >= catalog_.entries.size()) {
        phase_ = Phase::Done;
        finished_ = true;
        StreamEvent event{};
        event.kind = StreamEventKind::Complete;
        return event;
    }

    const catalog::SectionEntry& entry = catalog_.entries[section_index_];
    StreamEvent begin{};
    begin.kind = StreamEventKind::SectionBegin;
    begin.section = entry;

    if (entry.offset + entry.length > input_.size) {
        return core::Error{core::ErrorCode::SectionOutOfBounds, "section unavailable during streaming", static_cast<std::size_t>(entry.offset)};
    }

    const core::ByteSpan section_span{input_.data + entry.offset, static_cast<std::size_t>(entry.length)};
    const std::uint32_t schema_length = core::read_le_u32(section_span.data);
    if (schema_length > section_span.size) {
        return core::Error{core::ErrorCode::InvalidLength, "schema length invalid during streaming", static_cast<std::size_t>(entry.offset)};
    }

    ++section_index_;
    return begin;
}

core::Result<StreamEvent> StreamReader::next() {
    switch (phase_) {
    case Phase::Header:
        return read_header();
    case Phase::Catalog:
        return read_catalog();
    case Phase::Sections:
        return read_section();
    case Phase::Done:
        finished_ = true;
        return StreamEvent{StreamEventKind::Complete};
    }
    return core::Error{core::ErrorCode::InternalError, "invalid stream phase", 0};
}

} // namespace tessera::stream
