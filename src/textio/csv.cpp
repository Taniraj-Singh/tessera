#include "tessera/textio/csv.hpp"

#include "tessera/reflect/field_ref.hpp"

#include <cstdlib>

namespace tessera::textio {

namespace {

bool needs_quoting(const std::string& field) {
    return field.find(',') != std::string::npos || field.find('"') != std::string::npos ||
           field.find('\n') != std::string::npos || field.find('\r') != std::string::npos;
}

void append_field(std::string& out, const std::string& field) {
    if (!needs_quoting(field)) {
        out += field;
        return;
    }
    out.push_back('"');
    for (char character : field) {
        if (character == '"') {
            out += "\"\"";
        } else {
            out.push_back(character);
        }
    }
    out.push_back('"');
}

std::string cell_to_string(const schema::FieldDescriptor& field, const records::FieldValue& value) {
    const reflect::FieldRef ref(field, value);
    switch (field.type) {
    case schema::FieldType::Bool:
        return (ref.as_bool() != nullptr && *ref.as_bool()) ? "true" : "false";
    case schema::FieldType::U8:
        return ref.as_u8() != nullptr ? std::to_string(*ref.as_u8()) : "0";
    case schema::FieldType::I32:
        return ref.as_i32() != nullptr ? std::to_string(*ref.as_i32()) : "0";
    case schema::FieldType::U32:
        return ref.as_u32() != nullptr ? std::to_string(*ref.as_u32()) : "0";
    case schema::FieldType::I64:
        return ref.as_i64() != nullptr ? std::to_string(*ref.as_i64()) : "0";
    case schema::FieldType::U64:
    case schema::FieldType::TimestampMs:
        return ref.as_u64() != nullptr ? std::to_string(*ref.as_u64()) : "0";
    case schema::FieldType::F64:
        return ref.as_f64() != nullptr ? std::to_string(*ref.as_f64()) : "0";
    case schema::FieldType::Utf8:
        return ref.as_string() != nullptr ? *ref.as_string() : std::string{};
    case schema::FieldType::Bytes: {
        std::string hex;
        if (const std::vector<std::uint8_t>* bytes = ref.as_bytes()) {
            static const char* kHex = "0123456789abcdef";
            for (std::uint8_t byte : *bytes) {
                hex.push_back(kHex[(byte >> 4U) & 0xFU]);
                hex.push_back(kHex[byte & 0xFU]);
            }
        }
        return hex;
    }
    }
    return {};
}

// Splits one CSV record into fields, honoring quotes. Returns the offset of the
// next record, or std::string_view::npos at end of input.
std::size_t parse_record(std::string_view text, std::size_t start, std::vector<std::string>& out) {
    out.clear();
    std::string current;
    bool quoted = false;
    std::size_t pos = start;
    while (pos < text.size()) {
        const char character = text[pos];
        if (quoted) {
            if (character == '"') {
                if (pos + 1 < text.size() && text[pos + 1] == '"') {
                    current.push_back('"');
                    pos += 2;
                    continue;
                }
                quoted = false;
                ++pos;
                continue;
            }
            current.push_back(character);
            ++pos;
            continue;
        }
        if (character == '"') {
            quoted = true;
            ++pos;
            continue;
        }
        if (character == ',') {
            out.push_back(std::move(current));
            current.clear();
            ++pos;
            continue;
        }
        if (character == '\n' || character == '\r') {
            ++pos;
            if (character == '\r' && pos < text.size() && text[pos] == '\n') {
                ++pos;
            }
            out.push_back(std::move(current));
            return pos;
        }
        current.push_back(character);
        ++pos;
    }
    out.push_back(std::move(current));
    return std::string_view::npos;
}

core::Result<records::FieldValue> cell_from_string(schema::FieldType type, const std::string& cell) {
    switch (type) {
    case schema::FieldType::Bool:
        return records::FieldValue{cell == "true" || cell == "1"};
    case schema::FieldType::U8:
        return records::FieldValue{static_cast<std::uint8_t>(std::strtoul(cell.c_str(), nullptr, 10))};
    case schema::FieldType::I32:
        return records::FieldValue{static_cast<std::int32_t>(std::strtol(cell.c_str(), nullptr, 10))};
    case schema::FieldType::U32:
        return records::FieldValue{static_cast<std::uint32_t>(std::strtoul(cell.c_str(), nullptr, 10))};
    case schema::FieldType::I64:
        return records::FieldValue{static_cast<std::int64_t>(std::strtoll(cell.c_str(), nullptr, 10))};
    case schema::FieldType::U64:
    case schema::FieldType::TimestampMs:
        return records::FieldValue{static_cast<std::uint64_t>(std::strtoull(cell.c_str(), nullptr, 10))};
    case schema::FieldType::F64:
        return records::FieldValue{std::strtod(cell.c_str(), nullptr)};
    case schema::FieldType::Utf8:
        return records::FieldValue{cell};
    case schema::FieldType::Bytes: {
        std::vector<std::uint8_t> bytes;
        if (cell.size() % 2 != 0) {
            return core::Error{core::ErrorCode::InvalidLength, "odd-length hex in bytes cell", 0};
        }
        bytes.reserve(cell.size() / 2);
        auto nibble = [](char character) -> int {
            if (character >= '0' && character <= '9') {
                return character - '0';
            }
            if (character >= 'a' && character <= 'f') {
                return character - 'a' + 10;
            }
            if (character >= 'A' && character <= 'F') {
                return character - 'A' + 10;
            }
            return -1;
        };
        for (std::size_t index = 0; index + 1 < cell.size() + 1; index += 2) {
            if (index + 1 >= cell.size()) {
                break;
            }
            const int high = nibble(cell[index]);
            const int low = nibble(cell[index + 1]);
            if (high < 0 || low < 0) {
                return core::Error{core::ErrorCode::InvalidEnum, "invalid hex in bytes cell", index};
            }
            bytes.push_back(static_cast<std::uint8_t>((high << 4) | low));
        }
        return records::FieldValue{std::move(bytes)};
    }
    }
    return core::Error{core::ErrorCode::InvalidEnum, "unsupported field type", 0};
}

} // namespace

std::string section_to_csv(const model::SectionData& section) {
    std::string out;
    for (std::size_t column = 0; column < section.schema.fields.size(); ++column) {
        if (column != 0) {
            out.push_back(',');
        }
        append_field(out, section.schema.fields[column].name);
    }
    out.push_back('\n');

    for (const records::RecordRow& row : section.records.rows) {
        for (std::size_t column = 0; column < row.values.size() && column < section.schema.fields.size(); ++column) {
            if (column != 0) {
                out.push_back(',');
            }
            append_field(out, cell_to_string(section.schema.fields[column], row.values[column]));
        }
        out.push_back('\n');
    }
    return out;
}

core::Result<std::vector<records::RecordRow>> rows_from_csv(std::string_view text, const schema::SchemaBlock& schema) {
    std::vector<records::RecordRow> rows;
    std::vector<std::string> fields;
    std::size_t pos = 0;
    bool header_skipped = false;

    while (pos != std::string_view::npos && pos < text.size()) {
        pos = parse_record(text, pos, fields);
        if (!header_skipped) {
            header_skipped = true;
            continue;
        }
        if (fields.size() == 1 && fields.front().empty()) {
            continue; // blank trailing line
        }
        records::RecordRow row{};
        for (std::size_t column = 0; column < fields.size() && column < schema.fields.size(); ++column) {
            auto value = cell_from_string(schema.fields[column].type, fields[column]);
            if (!value) {
                return value.error();
            }
            row.values.push_back(std::move(*value));
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

} // namespace tessera::textio
