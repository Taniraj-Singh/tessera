#pragma once

#include "tessera/model/container.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tessera::graph {

enum class NodeKind : std::uint8_t {
    Container,
    Section,
    Schema,
    Field,
    RecordSet,
    Row,
    Value,
};

// A node in the materialized object graph. The graph mirrors the container's
// logical structure (container → sections → schema/records → fields/rows →
// values) so traversal, diffing, and rendering code can walk one uniform tree
// instead of the heterogeneous model types.
struct Node {
    NodeKind kind{NodeKind::Container};
    std::string label;
    std::size_t index{0}; // position within the parent
    std::vector<Node> children;
};

[[nodiscard]] std::string_view node_kind_name(NodeKind kind) noexcept;

// Builds the graph for a container. When include_rows is false the record rows
// and values are omitted, yielding a lightweight structural skeleton.
[[nodiscard]] Node build_graph(const model::Container& container, bool include_rows = true);

[[nodiscard]] std::size_t node_count(const Node& root) noexcept;
[[nodiscard]] std::size_t max_depth(const Node& root) noexcept;

} // namespace tessera::graph
