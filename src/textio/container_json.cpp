#include "tessera/textio/container_json.hpp"

#include "tessera/reflect/field_ref.hpp"
#include "tessera/reflect/type_info.hpp"

#include <cstdint>

namespace tessera::textio {

namespace {

core::Result<schema::FieldType> field_type_from_name(std::string_view name) {
    if (name == "bool") {
        return schema::FieldType::Bool;
    }
    if (name == "u8") {
        return schema::FieldType::U8;
    }
    if (name == "i32") {
        return schema::FieldType::I32;
    }
    if (name == "u32") {
        return schema::FieldType::U32;
    }
    if (name == "i64") {
        return schema::FieldType::I64;
    }
    if (name == "u64") {
        return schema::FieldType::U64;
    }
    if (name == "f64") {
        return schema::FieldType::F64;
    }
    if (name == "utf8") {
        return schema::FieldType::Utf8;
    }
    if (name == "bytes") {
        return schema::FieldType::Bytes;
    }
    if (name == "timestamp_ms") {
        return schema::FieldType::TimestampMs;
    }
    return core::Error{core::ErrorCode::InvalidEnum, "unknown field type name", 0};
}

JsonValue value_to_json(const schema::FieldDescriptor& field, const records::FieldValue& value) {
    const reflect::FieldRef ref(field, value);
    switch (field.type) {
    case schema::FieldType::Bool:
        return JsonValue{ref.as_bool() != nullptr && *ref.as_bool()};
    case schema::FieldType::U8:
        return JsonValue{ref.as_u8() != nullptr ? static_cast<double>(*ref.as_u8()) : 0.0};
    case schema::FieldType::I32:
        return JsonValue{ref.as_i32() != nullptr ? static_cast<double>(*ref.as_i32()) : 0.0};
    case schema::FieldType::U32:
        return JsonValue{ref.as_u32() != nullptr ? static_cast<double>(*ref.as_u32()) : 0.0};
    case schema::FieldType::I64:
        return JsonValue{ref.as_i64() != nullptr ? static_cast<double>(*ref.as_i64()) : 0.0};
    case schema::FieldType::U64:
    case schema::FieldType::TimestampMs:
        return JsonValue{ref.as_u64() != nullptr ? static_cast<double>(*ref.as_u64()) : 0.0};
    case schema::FieldType::F64:
        return JsonValue{ref.as_f64() != nullptr ? *ref.as_f64() : 0.0};
    case schema::FieldType::Utf8:
        return JsonValue{ref.as_string() != nullptr ? *ref.as_string() : std::string{}};
    case schema::FieldType::Bytes: {
        JsonValue array = JsonValue::array();
        if (const std::vector<std::uint8_t>* bytes = ref.as_bytes()) {
            for (std::uint8_t byte : *bytes) {
                array.push_back(JsonValue{static_cast<double>(byte)});
            }
        }
        return array;
    }
    }
    return JsonValue{};
}

core::Result<records::FieldValue> value_from_json(schema::FieldType type, const JsonValue& value) {
    switch (type) {
    case schema::FieldType::Bool:
        if (!value.is_bool()) {
            return core::Error{core::ErrorCode::SchemaMismatch, "expected bool", 0};
        }
        return records::FieldValue{value.as_bool()};
    case schema::FieldType::U8:
        return records::FieldValue{static_cast<std::uint8_t>(value.as_number())};
    case schema::FieldType::I32:
        return records::FieldValue{static_cast<std::int32_t>(value.as_number())};
    case schema::FieldType::U32:
        return records::FieldValue{static_cast<std::uint32_t>(value.as_number())};
    case schema::FieldType::I64:
        return records::FieldValue{static_cast<std::int64_t>(value.as_number())};
    case schema::FieldType::U64:
    case schema::FieldType::TimestampMs:
        return records::FieldValue{static_cast<std::uint64_t>(value.as_number())};
    case schema::FieldType::F64:
        return records::FieldValue{value.as_number()};
    case schema::FieldType::Utf8:
        if (!value.is_string()) {
            return core::Error{core::ErrorCode::SchemaMismatch, "expected string", 0};
        }
        return records::FieldValue{value.as_string()};
    case schema::FieldType::Bytes: {
        if (!value.is_array()) {
            return core::Error{core::ErrorCode::SchemaMismatch, "expected byte array", 0};
        }
        std::vector<std::uint8_t> bytes;
        bytes.reserve(value.items().size());
        for (const JsonValue& element : value.items()) {
            bytes.push_back(static_cast<std::uint8_t>(element.as_number()));
        }
        return records::FieldValue{std::move(bytes)};
    }
    }
    return core::Error{core::ErrorCode::InvalidEnum, "unsupported field type", 0};
}

} // namespace

JsonValue container_to_json(const model::Container& container) {
    JsonValue root = JsonValue::object();
    root.set("version", JsonValue{static_cast<double>(container.header.version)});
    root.set("created_unix_ms", JsonValue{static_cast<double>(container.header.created_unix_ms)});

    JsonValue sections = JsonValue::array();
    for (const model::SectionData& section : container.sections) {
        JsonValue node = JsonValue::object();
        node.set("id", JsonValue{static_cast<double>(section.entry.id)});
        node.set("name", JsonValue{section.entry.name});

        JsonValue fields = JsonValue::array();
        for (const schema::FieldDescriptor& field : section.schema.fields) {
            JsonValue descriptor = JsonValue::object();
            descriptor.set("name", JsonValue{field.name});
            descriptor.set("type", JsonValue{std::string(reflect::type_name(field.type))});
            descriptor.set("max_length", JsonValue{static_cast<double>(field.max_length)});
            fields.push_back(std::move(descriptor));
        }
        node.set("fields", std::move(fields));

        JsonValue rows = JsonValue::array();
        for (const records::RecordRow& row : section.records.rows) {
            JsonValue values = JsonValue::array();
            for (std::size_t column = 0; column < row.values.size() && column < section.schema.fields.size(); ++column) {
                values.push_back(value_to_json(section.schema.fields[column], row.values[column]));
            }
            rows.push_back(std::move(values));
        }
        node.set("rows", std::move(rows));
        sections.push_back(std::move(node));
    }
    root.set("sections", std::move(sections));
    return root;
}

core::Result<model::Container> container_from_json(const JsonValue& root) {
    if (!root.is_object()) {
        return core::Error{core::ErrorCode::SchemaMismatch, "container json must be an object", 0};
    }
    model::Container container{};
    container.header.magic = core::kContainerMagic;
    container.header.version = core::kContainerVersion;
    if (const JsonValue* version = root.find("version"); version != nullptr && version->is_number()) {
        container.header.version = static_cast<std::uint16_t>(version->as_number());
    }
    if (const JsonValue* created = root.find("created_unix_ms"); created != nullptr && created->is_number()) {
        container.header.created_unix_ms = static_cast<std::uint64_t>(created->as_number());
    }

    const JsonValue* sections = root.find("sections");
    if (sections == nullptr || !sections->is_array()) {
        return core::Error{core::ErrorCode::SchemaMismatch, "missing sections array", 0};
    }

    for (const JsonValue& node : sections->items()) {
        if (!node.is_object()) {
            return core::Error{core::ErrorCode::SchemaMismatch, "section must be an object", 0};
        }
        model::SectionData section{};
        if (const JsonValue* id = node.find("id"); id != nullptr && id->is_number()) {
            section.entry.id = static_cast<std::uint16_t>(id->as_number());
        }
        if (const JsonValue* name = node.find("name"); name != nullptr && name->is_string()) {
            section.entry.name = name->as_string();
        }
        section.schema.section_id = section.entry.id;
        section.records.section_id = section.entry.id;

        const JsonValue* fields = node.find("fields");
        if (fields == nullptr || !fields->is_array()) {
            return core::Error{core::ErrorCode::SchemaMismatch, "section missing fields", 0};
        }
        for (const JsonValue& descriptor : fields->items()) {
            if (!descriptor.is_object()) {
                return core::Error{core::ErrorCode::SchemaMismatch, "field must be an object", 0};
            }
            schema::FieldDescriptor field{};
            if (const JsonValue* name = descriptor.find("name"); name != nullptr && name->is_string()) {
                field.name = name->as_string();
            }
            const JsonValue* type = descriptor.find("type");
            if (type == nullptr || !type->is_string()) {
                return core::Error{core::ErrorCode::SchemaMismatch, "field missing type", 0};
            }
            auto parsed = field_type_from_name(type->as_string());
            if (!parsed) {
                return parsed.error();
            }
            field.type = *parsed;
            if (const JsonValue* max_length = descriptor.find("max_length"); max_length != nullptr && max_length->is_number()) {
                field.max_length = static_cast<std::uint16_t>(max_length->as_number());
            }
            section.schema.fields.push_back(std::move(field));
        }

        if (const JsonValue* rows = node.find("rows"); rows != nullptr && rows->is_array()) {
            for (const JsonValue& row_node : rows->items()) {
                if (!row_node.is_array()) {
                    return core::Error{core::ErrorCode::SchemaMismatch, "row must be an array", 0};
                }
                records::RecordRow row{};
                for (std::size_t column = 0; column < row_node.items().size() && column < section.schema.fields.size(); ++column) {
                    auto value = value_from_json(section.schema.fields[column].type, row_node.items()[column]);
                    if (!value) {
                        return value.error();
                    }
                    row.values.push_back(std::move(*value));
                }
                section.records.rows.push_back(std::move(row));
            }
        }
        section.records.row_count = static_cast<std::uint32_t>(section.records.rows.size());

        container.catalog.entries.push_back(section.entry);
        container.sections.push_back(std::move(section));
    }
    container.header.section_count = static_cast<std::uint32_t>(container.sections.size());
    return container;
}

std::string container_to_json_string(const model::Container& container, bool pretty) {
    return container_to_json(container).dump(pretty);
}

core::Result<model::Container> container_from_json_string(std::string_view text) {
    auto json = parse_json(text);
    if (!json) {
        return json.error();
    }
    return container_from_json(*json);
}

} // namespace tessera::textio
