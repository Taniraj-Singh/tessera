#include "support/test_framework.hpp"
#include "support/test_helpers.hpp"

#include "tessera/schema/schema_compiler.hpp"
#include "tessera/schema/type_system.hpp"
#include "tessera/validate/constraint_validator.hpp"
#include "tessera/validate/semantic_validator.hpp"

#include <cstdint>
#include <string>

namespace tessera::test {

namespace {

model::Container make_sequence_container() {
    model::Container container{};
    container.header.magic = core::kContainerMagic;
    container.header.version = core::kContainerVersion;
    container.header.section_count = 1;

    model::SectionData section{};
    section.entry.id = 5;
    section.entry.name = "series";
    section.schema.section_id = 5;
    section.schema.fields = {
        {"seq", schema::FieldType::U32, 0},
        {"tag", schema::FieldType::Utf8, 32},
    };
    section.records.section_id = 5;

    const std::uint32_t values[] = {10, 5, 7}; // deliberately non-monotonic, seq 7 duplicated below
    const char* tags[] = {"a", "b", "a"};
    for (int index = 0; index < 3; ++index) {
        records::RecordRow row{};
        row.values = {values[index], std::string{tags[index]}};
        section.records.rows.push_back(std::move(row));
    }
    section.records.row_count = 3;

    container.sections.push_back(std::move(section));
    container.catalog.entries.push_back(container.sections.front().entry);
    return container;
}

} // namespace

int run_schema_tests() {
    int before = g_failures;

    {
        TESSERA_EXPECT_TRUE(schema::is_widening(schema::FieldType::U8, schema::FieldType::U32));
        TESSERA_EXPECT_TRUE(!schema::is_widening(schema::FieldType::U32, schema::FieldType::U8));
        TESSERA_EXPECT_TRUE(schema::is_widening(schema::FieldType::I32, schema::FieldType::I64));
        TESSERA_EXPECT_TRUE(schema::is_widening(schema::FieldType::U32, schema::FieldType::I64));
        TESSERA_EXPECT_TRUE(!schema::is_widening(schema::FieldType::I64, schema::FieldType::U64));
        TESSERA_EXPECT_TRUE(schema::is_widening(schema::FieldType::U32, schema::FieldType::F64));
        TESSERA_EXPECT_TRUE(!schema::is_widening(schema::FieldType::U64, schema::FieldType::F64));
    }

    {
        schema::SchemaBlock base{};
        base.section_id = 1;
        base.fields = {{"a", schema::FieldType::U8, 0}, {"b", schema::FieldType::U32, 0}};

        schema::SchemaBlock same = base;
        TESSERA_EXPECT_TRUE(schema::compare_schema(base, same).overall == schema::Compatibility::Identical);

        schema::SchemaBlock widened = base;
        widened.fields[0].type = schema::FieldType::U32; // U8 -> U32
        TESSERA_EXPECT_TRUE(schema::compare_schema(base, widened).overall == schema::Compatibility::Compatible);

        schema::SchemaBlock extended = base;
        extended.fields.push_back({"c", schema::FieldType::Utf8, 16});
        TESSERA_EXPECT_TRUE(schema::compare_schema(base, extended).overall == schema::Compatibility::Compatible);

        schema::SchemaBlock shrunk = base;
        shrunk.fields.pop_back();
        TESSERA_EXPECT_TRUE(schema::compare_schema(base, shrunk).overall == schema::Compatibility::Incompatible);
    }

    {
        model::Container sample = make_sample_container();
        auto compiled = schema::compile(sample.sections.front().schema);
        TESSERA_EXPECT_TRUE(compiled.ok());
        TESSERA_EXPECT_EQ(compiled->field_count(), std::size_t{3});
        TESSERA_EXPECT_EQ(compiled->min_row_size(), std::size_t{1 + 4 + 2});
        TESSERA_EXPECT_TRUE(!compiled->all_fixed());
        TESSERA_EXPECT_TRUE(compiled->find_field("code") != nullptr);
        TESSERA_EXPECT_TRUE(compiled->find_field("absent") == nullptr);

        schema::SchemaBlock dup{};
        dup.section_id = 1;
        dup.fields = {{"x", schema::FieldType::U8, 0}, {"x", schema::FieldType::U8, 0}};
        TESSERA_EXPECT_TRUE(!schema::compile(dup).ok());
    }

    {
        model::Container sample = make_sample_container();
        auto report = validate::validate_semantics(sample, {});
        TESSERA_EXPECT_TRUE(report.valid);

        // Break the type of the second field's value.
        sample.sections[0].records.rows[0].values[1] = std::string{"not a u32"};
        auto broken = validate::validate_semantics(sample, {});
        TESSERA_EXPECT_TRUE(!broken.valid);
    }

    {
        model::Container series = make_sequence_container();

        std::vector<validate::Constraint> ok_rules = {
            {"series", "seq", validate::ConstraintKind::MinValue, 0},
            {"series", "tag", validate::ConstraintKind::NotEmpty, 0},
        };
        TESSERA_EXPECT_TRUE(validate::check_constraints(series, ok_rules).valid);

        std::vector<validate::Constraint> mono = {{"series", "seq", validate::ConstraintKind::Monotonic, 0}};
        TESSERA_EXPECT_TRUE(!validate::check_constraints(series, mono).valid); // 10,5,7 not monotonic

        std::vector<validate::Constraint> unique = {{"series", "tag", validate::ConstraintKind::Unique, 0}};
        TESSERA_EXPECT_TRUE(!validate::check_constraints(series, unique).valid); // "a" repeats

        std::vector<validate::Constraint> cap = {{"series", "seq", validate::ConstraintKind::MaxValue, 8}};
        TESSERA_EXPECT_TRUE(!validate::check_constraints(series, cap).valid); // 10 > 8

        std::vector<validate::Constraint> bad_section = {{"nope", "seq", validate::ConstraintKind::NotEmpty, 0}};
        TESSERA_EXPECT_TRUE(!validate::check_constraints(series, bad_section).valid);
    }

    return g_failures - before;
}

} // namespace tessera::test
