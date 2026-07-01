#pragma once

#include "tessera/core/types.hpp"
#include "tessera/header/container_header.hpp"
#include "tessera/catalog/section_catalog.hpp"

#include <functional>
#include <vector>

namespace tessera::stream {

enum class StreamEventKind {
    Header,
    Catalog,
    SectionBegin,
    SectionSchema,
    SectionRecords,
    SectionEnd,
    Complete,
    Error,
};

struct StreamEvent {
    StreamEventKind kind{StreamEventKind::Error};
    core::Error error{};
    header::ContainerHeader header{};
    catalog::Catalog catalog{};
    catalog::SectionEntry section{};
};

class StreamReader {
public:
    explicit StreamReader(core::ByteSpan input);

    [[nodiscard]] bool finished() const noexcept { return finished_; }

    core::Result<StreamEvent> next();

private:
    core::Result<StreamEvent> read_header();
    core::Result<StreamEvent> read_catalog();
    core::Result<StreamEvent> read_section();

    core::ByteSpan input_;
    std::size_t section_index_{0};
    enum class Phase { Header, Catalog, Sections, Done } phase_{Phase::Header};
    bool finished_{false};
    header::ContainerHeader header_{};
    catalog::Catalog catalog_{};
};

} // namespace tessera::stream
