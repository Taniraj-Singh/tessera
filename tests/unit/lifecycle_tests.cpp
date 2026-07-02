#include "support/test_framework.hpp"
#include "support/test_helpers.hpp"

#include "tessera/recovery/repair.hpp"
#include "tessera/resource/resource_manager.hpp"
#include "tessera/txn/transaction.hpp"

#include <cstdint>

namespace tessera::test {

namespace {

model::SectionData make_named_section(std::uint16_t id, const char* name) {
    model::SectionData section{};
    section.entry.id = id;
    section.entry.name = name;
    section.schema.section_id = id;
    section.schema.fields = {{"value", schema::FieldType::U32, 0}};
    section.records.section_id = id;
    records::RecordRow row{};
    row.values = {std::uint32_t{1}};
    section.records.rows.push_back(std::move(row));
    section.records.row_count = 1;
    return section;
}

} // namespace

int run_lifecycle_tests() {
    int before = g_failures;

    {
        txn::Transaction txn(make_sample_container());
        TESSERA_EXPECT_TRUE(txn.add_section(make_named_section(2, "extra")).ok());
        TESSERA_EXPECT_EQ(txn.section_count(), std::size_t{2});
        TESSERA_EXPECT_TRUE(!txn.add_section(make_named_section(2, "dup")).ok()); // duplicate id
        TESSERA_EXPECT_TRUE(txn.rename_section(1, "renamed").ok());
        TESSERA_EXPECT_TRUE(!txn.rename_section(1, "extra").ok()); // name collision

        auto committed = txn.commit();
        TESSERA_EXPECT_TRUE(committed.ok());
        TESSERA_EXPECT_EQ(committed->sections.size(), std::size_t{2});

        TESSERA_EXPECT_TRUE(txn.remove_section(2).ok());
        TESSERA_EXPECT_EQ(txn.section_count(), std::size_t{1});
        txn.rollback();
        TESSERA_EXPECT_EQ(txn.section_count(), std::size_t{1}); // base had 1 section
        TESSERA_EXPECT_EQ(txn.pending_ops(), std::size_t{0});
    }

    {
        // A container with deliberately inconsistent bookkeeping is repaired.
        model::Container broken = make_sample_container();
        broken.sections.front().schema.section_id = 99;   // mismatched id
        broken.sections.front().records.row_count = 7;     // wrong count
        broken.sections.push_back(model::SectionData{});   // unsalvageable (empty name)
        broken.header.section_count = 0;                   // wrong header count

        recovery::RepairReport report = recovery::repair(broken);
        TESSERA_EXPECT_EQ(report.sections_dropped, std::size_t{1});
        TESSERA_EXPECT_TRUE(report.ids_realigned >= 1);
        TESSERA_EXPECT_TRUE(report.rows_trimmed >= 1);
        TESSERA_EXPECT_TRUE(report.catalog_rebuilt);
        TESSERA_EXPECT_EQ(broken.sections.size(), std::size_t{1});
        TESSERA_EXPECT_EQ(broken.header.section_count, std::uint32_t{1});
        TESSERA_EXPECT_EQ(broken.sections.front().schema.section_id, broken.sections.front().entry.id);
    }

    {
        resource::ResourceManager manager(resource::Budget{100, 3});
        TESSERA_EXPECT_TRUE(manager.acquire(40).ok());
        TESSERA_EXPECT_TRUE(manager.acquire(40).ok());
        TESSERA_EXPECT_TRUE(!manager.acquire(40).ok()); // would exceed 100 bytes
        TESSERA_EXPECT_EQ(manager.used_bytes(), std::size_t{80});
        manager.release(40);
        TESSERA_EXPECT_EQ(manager.used_bytes(), std::size_t{40});

        {
            auto lease = resource::Lease::take(manager, 20);
            TESSERA_EXPECT_TRUE(lease.ok());
            TESSERA_EXPECT_EQ(manager.used_bytes(), std::size_t{60});
        }
        // Lease released at scope exit.
        TESSERA_EXPECT_EQ(manager.used_bytes(), std::size_t{40});

        resource::ResourceManager capped(resource::Budget{0, 1});
        TESSERA_EXPECT_TRUE(capped.acquire(1'000'000).ok()); // bytes unbounded
        TESSERA_EXPECT_TRUE(!capped.acquire(1).ok());        // handle budget = 1
    }

    return g_failures - before;
}

} // namespace tessera::test
