#include "tessera/convert/converter.hpp"

#include "tessera/parse/parser.hpp"
#include "tessera/serialize/serializer.hpp"

#include <iomanip>
#include <sstream>

namespace tessera::convert {

namespace {

std::string field_type_name(schema::FieldType type) {
    switch (type) {
    case schema::FieldType::Bool:
        return "bool";
    case schema::FieldType::U8:
        return "u8";
    case schema::FieldType::I32:
        return "i32";
    case schema::FieldType::U32:
        return "u32";
    case schema::FieldType::I64:
        return "i64";
    case schema::FieldType::U64:
        return "u64";
    case schema::FieldType::F64:
        return "f64";
    case schema::FieldType::Utf8:
        return "utf8";
    case schema::FieldType::Bytes:
        return "bytes";
    case schema::FieldType::TimestampMs:
        return "timestamp_ms";
    }
    return "unknown";
}

} // namespace

core::Result<std::string> convert_container(const model::Container& container, const ConvertOptions& options) {
    switch (options.format) {
    case OutputFormat::JsonSummary: {
        std::ostringstream out;
        out << "{\n";
        out << "  \"magic\": " << container.header.magic << ",\n";
        out << "  \"version\": " << container.header.version << ",\n";
        out << "  \"sections\": " << container.sections.size() << ",\n";
        out << "  \"entries\": [\n";
        for (std::size_t index = 0; index < container.sections.size(); ++index) {
            const model::SectionData& section = container.sections[index];
            out << "    {\n";
            out << "      \"id\": " << section.entry.id << ",\n";
            out << "      \"name\": \"" << section.entry.name << "\",\n";
            out << "      \"rows\": " << section.records.rows.size() << ",\n";
            out << "      \"fields\": [";
            for (std::size_t field_index = 0; field_index < section.schema.fields.size(); ++field_index) {
                if (field_index != 0) {
                    out << ", ";
                }
                out << "\"" << section.schema.fields[field_index].name << ":" << field_type_name(section.schema.fields[field_index].type) << "\"";
            }
            out << "]\n";
            out << "    }";
            if (index + 1 < container.sections.size()) {
                out << ",";
            }
            out << "\n";
        }
        out << "  ]\n";
        out << "}\n";
        return out.str();
    }
    case OutputFormat::HexDump: {
        auto bytes = serialize::serialize_container(container, {});
        if (!bytes) {
            return bytes.error();
        }
        std::ostringstream out;
        out << std::hex << std::setfill('0');
        for (std::size_t index = 0; index < bytes->size(); ++index) {
            out << std::setw(2) << static_cast<unsigned>((*bytes)[index]);
            if ((index + 1) % 16 == 0) {
                out << '\n';
            } else {
                out << ' ';
            }
        }
        out << '\n';
        return out.str();
    }
    case OutputFormat::CanonicalBinary:
        return core::Error{core::ErrorCode::InternalError, "use convert_bytes for binary output", 0};
    }
    return core::Error{core::ErrorCode::InvalidEnum, "unknown output format", 0};
}

core::Result<std::vector<std::uint8_t>> convert_bytes(core::ByteSpan input, const ConvertOptions& options) {
    auto container = parse::parse_container(input, {});
    if (!container) {
        return container.error();
    }

    if (options.format == OutputFormat::CanonicalBinary) {
        return serialize::serialize_container(*container, {});
    }

    auto text = convert_container(*container, options);
    if (!text) {
        return text.error();
    }
    const std::string& value = *text;
    return std::vector<std::uint8_t>(value.begin(), value.end());
}

} // namespace tessera::convert
