#include "support/test_framework.hpp"
#include "support/test_helpers.hpp"

#include "tessera/diff/section_diff.hpp"
#include "tessera/index/ordered_index.hpp"
#include "tessera/query/predicate.hpp"

#include <cstdint>
#include <string>

namespace tessera::test {

namespace {

model::SectionData make_series_section() {
    model::SectionData section{};
    section.entry.id = 7;
    section.entry.name = "series";
    section.schema.section_id = 7;
    section.schema.fields = {
        {"seq", schema::FieldType::U32, 0},
        {"tag", schema::FieldType::Utf8, 16},
    };
    section.records.section_id = 7;
    const std::uint32_t seqs[] = {10, 20, 30, 40};
    const char* tags[] = {"a", "b", "a", "c"};
    for (int index = 0; index < 4; ++index) {
        records::RecordRow row{};
        row.values = {seqs[index], std::string{tags[index]}};
        section.records.rows.push_back(std::move(row));
    }
    section.records.row_count = 4;
    return section;
}

} // namespace

int run_query_tests() {
    int before = g_failures;

    {
        model::SectionData section = make_series_section();

        std::vector<query::Predicate> gt = {query::Predicate::numeric("seq", query::CompareOp::Gt, 15)};
        TESSERA_EXPECT_EQ(query::count_matching(section, gt), std::size_t{3}); // 20,30,40

        std::vector<query::Predicate> combo = {
            query::Predicate::numeric("seq", query::CompareOp::Ge, 20),
            query::Predicate::string_value("tag", query::CompareOp::Eq, "a"),
        };
        auto rows = query::select_rows(section, combo);
        TESSERA_EXPECT_EQ(rows.size(), std::size_t{1}); // seq=30, tag=a
        TESSERA_EXPECT_EQ(rows.front(), std::size_t{2});

        model::SectionData filtered = query::filter_section(section, gt);
        TESSERA_EXPECT_EQ(filtered.records.rows.size(), std::size_t{3});
        TESSERA_EXPECT_EQ(filtered.records.row_count, std::uint32_t{3});

        std::vector<query::Predicate> missing = {query::Predicate::numeric("nope", query::CompareOp::Eq, 1)};
        TESSERA_EXPECT_EQ(query::count_matching(section, missing), std::size_t{0});
    }

    {
        index::OrderedIndex index;
        index.insert(50, 5);
        index.insert(10, 1);
        index.insert(30, 3);
        index.insert(20, 2);
        index.insert(40, 4);
        TESSERA_EXPECT_TRUE(index.find(30) == nullptr); // not built yet
        index.build();
        const std::uint32_t* found = index.find(30);
        TESSERA_EXPECT_TRUE(found != nullptr && *found == 3U);
        TESSERA_EXPECT_TRUE(index.find(999) == nullptr);
        auto values = index.range(20, 40);
        TESSERA_EXPECT_EQ(values.size(), std::size_t{3}); // 20,30,40 -> 2,3,4
        TESSERA_EXPECT_EQ(values.front(), std::uint32_t{2});
        TESSERA_EXPECT_EQ(values.back(), std::uint32_t{4});
    }

    {
        model::SectionData base = make_series_section();
        model::SectionData updated = make_series_section();
        updated.records.rows[1].values = {std::uint32_t{99}, std::string{"b"}}; // modify row 1
        records::RecordRow extra{};
        extra.values = {std::uint32_t{50}, std::string{"d"}};
        updated.records.rows.push_back(std::move(extra)); // add a row
        updated.records.row_count = 5;

        diff::SectionDiff sd = diff::diff_sections(base, updated);
        TESSERA_EXPECT_EQ(sd.modified, std::size_t{1});
        TESSERA_EXPECT_EQ(sd.added, std::size_t{1});
        TESSERA_EXPECT_EQ(sd.removed, std::size_t{0});
        TESSERA_EXPECT_TRUE(!sd.identical());
        TESSERA_EXPECT_TRUE(!sd.schema_changed);

        diff::SectionDiff same = diff::diff_sections(base, base);
        TESSERA_EXPECT_TRUE(same.identical());
    }

    {
        model::Container base = make_sample_container();
        model::Container updated = make_sample_container();
        updated.sections.front().records.rows.front().values = {false, std::uint32_t{7}, std::string{"x"}};
        diff::ContainerDiff cd = diff::diff_containers(base, updated);
        TESSERA_EXPECT_EQ(cd.sections_changed, std::size_t{1});
        TESSERA_EXPECT_EQ(cd.total_rows_modified, std::size_t{1});
        TESSERA_EXPECT_EQ(cd.sections_added, std::size_t{0});
    }

    return g_failures - before;
}

} // namespace tessera::test
