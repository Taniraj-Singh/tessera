#include "support/test_framework.hpp"
#include "support/test_helpers.hpp"

#include "tessera/metadata/registry.hpp"
#include "tessera/reflect/field_ref.hpp"
#include "tessera/reflect/type_info.hpp"

#include <cstdint>
#include <string>

namespace tessera::test {

int run_reflect_tests() {
    int before = g_failures;

    {
        const reflect::TypeInfo& u32 = reflect::type_info(schema::FieldType::U32);
        TESSERA_EXPECT_TRUE(u32.fixed_size);
        TESSERA_EXPECT_EQ(u32.wire_size, std::size_t{4});
        TESSERA_EXPECT_TRUE(u32.numeric && !u32.is_signed);

        const reflect::TypeInfo& utf8 = reflect::type_info(schema::FieldType::Utf8);
        TESSERA_EXPECT_TRUE(!utf8.fixed_size && utf8.aggregate);
        TESSERA_EXPECT_EQ(reflect::fixed_wire_size(schema::FieldType::Utf8), std::size_t{0});
        TESSERA_EXPECT_EQ(reflect::fixed_wire_size(schema::FieldType::TimestampMs), std::size_t{8});
    }

    {
        schema::FieldDescriptor code{"code", schema::FieldType::U32, 0};
        records::FieldValue value{std::uint32_t{7}};
        reflect::FieldRef ref(code, value);
        TESSERA_EXPECT_TRUE(ref.matches_type());
        const std::uint32_t* raw = ref.as_u32();
        TESSERA_EXPECT_TRUE(raw != nullptr && *raw == 7U);
        TESSERA_EXPECT_TRUE(ref.as_string() == nullptr);

        // A value whose variant alternative disagrees with the descriptor fails.
        records::FieldValue wrong{std::string{"oops"}};
        reflect::FieldRef mismatched(code, wrong);
        TESSERA_EXPECT_TRUE(!mismatched.matches_type());

        schema::FieldDescriptor label{"label", schema::FieldType::Utf8, 64};
        records::FieldValue text{std::string{"hello"}};
        reflect::FieldRef text_ref(label, text);
        TESSERA_EXPECT_EQ(text_ref.encoded_size(), std::size_t{2 + 5});
    }

    {
        model::Container container = make_sample_container();
        metadata::MetadataRegistry registry = metadata::MetadataRegistry::build(container);
        TESSERA_EXPECT_EQ(registry.size(), std::size_t{1});
        const metadata::SectionDescriptor* events = registry.by_name("events");
        TESSERA_EXPECT_TRUE(events != nullptr);
        TESSERA_EXPECT_EQ(events->fields.size(), std::size_t{3});
        TESSERA_EXPECT_EQ(events->row_count, std::uint32_t{1});
        TESSERA_EXPECT_TRUE(registry.by_id(1) != nullptr);
        TESSERA_EXPECT_TRUE(registry.by_id(7) == nullptr);
        TESSERA_EXPECT_EQ(registry.total_field_count(), std::size_t{3});
    }

    return g_failures - before;
}

} // namespace tessera::test
