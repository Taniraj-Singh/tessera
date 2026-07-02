#include "support/test_framework.hpp"
#include "support/test_helpers.hpp"

#include "tessera/graph/object_graph.hpp"
#include "tessera/profile/profiler.hpp"
#include "tessera/stats/statistics.hpp"
#include "tessera/visit/visitor.hpp"

#include <cstdint>

namespace tessera::test {

namespace {

class CountingVisitor final : public visit::Visitor {
public:
    std::size_t sections{0};
    std::size_t fields{0};
    std::size_t rows{0};

    void enter_section(const model::SectionData&, std::size_t) override { ++sections; }
    void visit_field(const schema::FieldDescriptor&, std::size_t) override { ++fields; }
    void visit_row(const records::RecordRow&, std::size_t) override { ++rows; }
};

} // namespace

int run_introspection_tests() {
    int before = g_failures;

    model::Container sample = make_sample_container();

    {
        graph::Node root = graph::build_graph(sample, true);
        TESSERA_EXPECT_TRUE(root.kind == graph::NodeKind::Container);
        TESSERA_EXPECT_EQ(root.children.size(), std::size_t{1}); // one section
        // container > section > {schema>3 fields, records>1 row>3 values}
        TESSERA_EXPECT_TRUE(graph::node_count(root) > 8);
        TESSERA_EXPECT_TRUE(graph::max_depth(root) >= 5);

        graph::Node skeleton = graph::build_graph(sample, false);
        TESSERA_EXPECT_TRUE(graph::node_count(skeleton) < graph::node_count(root));
    }

    {
        CountingVisitor visitor;
        visit::traverse(sample, visitor);
        TESSERA_EXPECT_EQ(visitor.sections, std::size_t{1});
        TESSERA_EXPECT_EQ(visitor.fields, std::size_t{3});
        TESSERA_EXPECT_EQ(visitor.rows, std::size_t{1});
    }

    {
        stats::ContainerStats stats = stats::collect(sample);
        TESSERA_EXPECT_EQ(stats.sections, std::size_t{1});
        TESSERA_EXPECT_EQ(stats.total_fields, std::size_t{3});
        TESSERA_EXPECT_EQ(stats.total_rows, std::size_t{1});
        TESSERA_EXPECT_EQ(stats.largest_section_rows, std::size_t{1});
        TESSERA_EXPECT_TRUE(stats.total_value_bytes > 0);
        // The sample has a Bool, a U32, and a Utf8 field.
        TESSERA_EXPECT_EQ(stats.type_counts[static_cast<std::size_t>(schema::FieldType::Bool)], std::size_t{1});
        TESSERA_EXPECT_EQ(stats.type_counts[static_cast<std::size_t>(schema::FieldType::U32)], std::size_t{1});
        TESSERA_EXPECT_EQ(stats.type_counts[static_cast<std::size_t>(schema::FieldType::Utf8)], std::size_t{1});
    }

    {
        profile::Profiler profiler;
        profiler.count("parses");
        profiler.count("parses", 4);
        profiler.observe("row_bytes", 10);
        profiler.observe("row_bytes", 20);
        profiler.observe("row_bytes", 30);
        TESSERA_EXPECT_EQ(profiler.counter("parses"), std::uint64_t{5});
        TESSERA_EXPECT_EQ(profiler.counter("missing"), std::uint64_t{0});
        const profile::Stat* stat = profiler.stat("row_bytes");
        TESSERA_EXPECT_TRUE(stat != nullptr);
        TESSERA_EXPECT_EQ(stat->count, std::uint64_t{3});
        TESSERA_EXPECT_EQ(stat->sum, std::uint64_t{60});
        TESSERA_EXPECT_EQ(stat->minimum, std::uint64_t{10});
        TESSERA_EXPECT_EQ(stat->maximum, std::uint64_t{30});
        TESSERA_EXPECT_TRUE(stat->mean() > 19.0 && stat->mean() < 21.0);
    }

    return g_failures - before;
}

} // namespace tessera::test
