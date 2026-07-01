#pragma once

#include "tessera/catalog/section_catalog.hpp"
#include "tessera/header/container_header.hpp"
#include "tessera/records/record_batch.hpp"
#include "tessera/schema/schema_block.hpp"

#include <map>
#include <vector>

namespace tessera::model {

struct SectionData {
    catalog::SectionEntry entry;
    schema::SchemaBlock schema;
    records::RecordBatch records;
};

struct Container {
    header::ContainerHeader header;
    catalog::Catalog catalog;
    std::vector<SectionData> sections;
};

} // namespace tessera::model
