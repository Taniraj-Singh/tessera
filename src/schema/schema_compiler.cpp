#include "tessera/schema/schema_compiler.hpp"

#include "tessera/reflect/type_info.hpp"

namespace tessera::schema {

const CompiledField* CompiledSchema::find_field(std::string_view name) const {
    const std::uint32_t* slot = field_by_name_.find(name);
    if (slot == nullptr || *slot >= fields_.size()) {
        return nullptr;
    }
    return &fields_[*slot];
}

core::Result<CompiledSchema> compile(const SchemaBlock& schema) {
    CompiledSchema compiled{};
    compiled.section_id_ = schema.section_id;
    compiled.fields_.reserve(schema.fields.size());
    compiled.field_by_name_ = index::StringIndex(schema.fields.size());

    for (std::size_t index = 0; index < schema.fields.size(); ++index) {
        const FieldDescriptor& field = schema.fields[index];
        if (compiled.field_by_name_.contains(field.name)) {
            return core::Error{core::ErrorCode::SchemaMismatch, "duplicate field name in schema", index};
        }

        const reflect::TypeInfo& info = reflect::type_info(field.type);
        CompiledField entry{};
        entry.descriptor = field;
        entry.fixed = info.fixed_size;
        if (info.fixed_size) {
            entry.wire_size = info.wire_size;
        } else {
            // Variable fields cost at least their length prefix: 2 bytes for
            // Utf8 (u16 length), 4 bytes for Bytes (u32 length).
            entry.wire_size = field.type == FieldType::Utf8 ? 2U : 4U;
            compiled.all_fixed_ = false;
        }

        compiled.min_row_size_ += entry.wire_size;
        compiled.field_by_name_.insert(field.name, static_cast<std::uint32_t>(index));
        compiled.fields_.push_back(std::move(entry));
    }

    return compiled;
}

} // namespace tessera::schema
