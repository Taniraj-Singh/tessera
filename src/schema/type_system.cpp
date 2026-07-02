#include "tessera/schema/type_system.hpp"

#include <array>

namespace tessera::schema {

namespace {

// Classifies a numeric field so widening can be decided by (signedness, width).
struct Numeric {
    bool numeric{false};
    bool is_signed{false};
    int width_bits{0};
    bool floating{false};
};

Numeric classify(FieldType type) noexcept {
    switch (type) {
    case FieldType::U8:
        return {true, false, 8, false};
    case FieldType::U32:
        return {true, false, 32, false};
    case FieldType::U64:
    case FieldType::TimestampMs:
        return {true, false, 64, false};
    case FieldType::I32:
        return {true, true, 32, false};
    case FieldType::I64:
        return {true, true, 64, false};
    case FieldType::F64:
        return {true, true, 64, true};
    case FieldType::Bool:
    case FieldType::Utf8:
    case FieldType::Bytes:
        return {false, false, 0, false};
    }
    return {false, false, 0, false};
}

} // namespace

bool is_widening(FieldType from, FieldType to) noexcept {
    if (from == to) {
        return true;
    }
    // TimestampMs and U64 share storage and semantics.
    if ((from == FieldType::U64 && to == FieldType::TimestampMs) ||
        (from == FieldType::TimestampMs && to == FieldType::U64)) {
        return true;
    }

    const Numeric a = classify(from);
    const Numeric b = classify(to);
    if (!a.numeric || !b.numeric) {
        return false;
    }

    if (b.floating) {
        // Integers up to 32 bits convert to double without loss; 64-bit does not.
        return !a.floating && a.width_bits <= 32;
    }
    if (a.floating) {
        return false; // double never widens to an integer type
    }

    if (a.is_signed == b.is_signed) {
        return a.width_bits <= b.width_bits;
    }
    if (!a.is_signed && b.is_signed) {
        // Unsigned fits into a wider signed type (U8->I32, U32->I64).
        return a.width_bits < b.width_bits;
    }
    return false; // signed never widens into unsigned
}

Compatibility worse_of(Compatibility lhs, Compatibility rhs) noexcept {
    auto rank = [](Compatibility value) {
        switch (value) {
        case Compatibility::Identical:
            return 0;
        case Compatibility::Compatible:
            return 1;
        case Compatibility::Incompatible:
            return 2;
        }
        return 2;
    };
    return rank(lhs) >= rank(rhs) ? lhs : rhs;
}

Compatibility compare_field(const FieldDescriptor& old_field, const FieldDescriptor& new_field) noexcept {
    if (old_field.type == new_field.type && old_field.max_length == new_field.max_length &&
        old_field.name == new_field.name) {
        return Compatibility::Identical;
    }
    // A shrinking length cap can reject values the old schema accepted.
    if (old_field.max_length != 0 && (new_field.max_length == 0 || new_field.max_length >= old_field.max_length)) {
        // widening or lifting the cap is fine so far
    } else if (old_field.max_length != 0 && new_field.max_length < old_field.max_length) {
        return Compatibility::Incompatible;
    }
    if (is_widening(old_field.type, new_field.type)) {
        return Compatibility::Compatible;
    }
    return Compatibility::Incompatible;
}

SchemaDiff compare_schema(const SchemaBlock& old_schema, const SchemaBlock& new_schema) {
    SchemaDiff diff{};
    if (old_schema.section_id != new_schema.section_id) {
        diff.overall = Compatibility::Incompatible;
        diff.notes.emplace_back("section id changed");
        return diff;
    }

    const std::size_t shared = old_schema.fields.size() < new_schema.fields.size()
                                   ? old_schema.fields.size()
                                   : new_schema.fields.size();
    for (std::size_t index = 0; index < shared; ++index) {
        const Compatibility field_result = compare_field(old_schema.fields[index], new_schema.fields[index]);
        diff.overall = worse_of(diff.overall, field_result);
        if (field_result == Compatibility::Incompatible) {
            diff.notes.emplace_back("field " + old_schema.fields[index].name + " changed incompatibly");
        }
    }

    if (new_schema.fields.size() > old_schema.fields.size()) {
        diff.overall = worse_of(diff.overall, Compatibility::Compatible);
        diff.notes.emplace_back("new trailing fields added");
    } else if (new_schema.fields.size() < old_schema.fields.size()) {
        diff.overall = Compatibility::Incompatible;
        diff.notes.emplace_back("fields removed");
    }

    return diff;
}

} // namespace tessera::schema
