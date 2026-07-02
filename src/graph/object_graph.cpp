#include "tessera/graph/object_graph.hpp"

#include "tessera/reflect/type_info.hpp"

namespace tessera::graph {

std::string_view node_kind_name(NodeKind kind) noexcept {
    switch (kind) {
    case NodeKind::Container:
        return "container";
    case NodeKind::Section:
        return "section";
    case NodeKind::Schema:
        return "schema";
    case NodeKind::Field:
        return "field";
    case NodeKind::RecordSet:
        return "records";
    case NodeKind::Row:
        return "row";
    case NodeKind::Value:
        return "value";
    }
    return "unknown";
}

namespace {

Node build_schema_node(const schema::SchemaBlock& schema) {
    Node schema_node{NodeKind::Schema, "schema", 0, {}};
    schema_node.children.reserve(schema.fields.size());
    for (std::size_t index = 0; index < schema.fields.size(); ++index) {
        const schema::FieldDescriptor& field = schema.fields[index];
        Node field_node{NodeKind::Field, field.name, index, {}};
        schema_node.children.push_back(std::move(field_node));
    }
    return schema_node;
}

Node build_records_node(const model::SectionData& section, bool include_rows) {
    Node records_node{NodeKind::RecordSet, "records", 0, {}};
    if (!include_rows) {
        return records_node;
    }
    records_node.children.reserve(section.records.rows.size());
    for (std::size_t row_index = 0; row_index < section.records.rows.size(); ++row_index) {
        const records::RecordRow& row = section.records.rows[row_index];
        Node row_node{NodeKind::Row, "row", row_index, {}};
        row_node.children.reserve(row.values.size());
        for (std::size_t value_index = 0; value_index < row.values.size(); ++value_index) {
            std::string label = "value";
            if (value_index < section.schema.fields.size()) {
                label = std::string(reflect::type_name(section.schema.fields[value_index].type));
            }
            row_node.children.push_back(Node{NodeKind::Value, std::move(label), value_index, {}});
        }
        records_node.children.push_back(std::move(row_node));
    }
    return records_node;
}

} // namespace

Node build_graph(const model::Container& container, bool include_rows) {
    Node root{NodeKind::Container, "container", 0, {}};
    root.children.reserve(container.sections.size());
    for (std::size_t index = 0; index < container.sections.size(); ++index) {
        const model::SectionData& section = container.sections[index];
        Node section_node{NodeKind::Section, section.entry.name, index, {}};
        section_node.children.push_back(build_schema_node(section.schema));
        section_node.children.push_back(build_records_node(section, include_rows));
        root.children.push_back(std::move(section_node));
    }
    return root;
}

std::size_t node_count(const Node& root) noexcept {
    std::size_t total = 1;
    for (const Node& child : root.children) {
        total += node_count(child);
    }
    return total;
}

std::size_t max_depth(const Node& root) noexcept {
    std::size_t deepest = 0;
    for (const Node& child : root.children) {
        const std::size_t child_depth = max_depth(child);
        if (child_depth > deepest) {
            deepest = child_depth;
        }
    }
    return deepest + 1;
}

} // namespace tessera::graph
