#pragma once

#include "tessera/core/types.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tessera::textio {

// A minimal but complete JSON value model plus a recursive-descent parser and a
// serializer. Used by the text import/export layer to round-trip containers
// through JSON, and generally useful as a standalone JSON facility.
class JsonValue {
public:
    enum class Kind {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object,
    };

    JsonValue() : kind_(Kind::Null) {}
    explicit JsonValue(bool value) : kind_(Kind::Bool), bool_(value) {}
    explicit JsonValue(double value) : kind_(Kind::Number), number_(value) {}
    explicit JsonValue(std::string value) : kind_(Kind::String), string_(std::move(value)) {}

    static JsonValue array() {
        JsonValue value;
        value.kind_ = Kind::Array;
        return value;
    }
    static JsonValue object() {
        JsonValue value;
        value.kind_ = Kind::Object;
        return value;
    }

    [[nodiscard]] Kind kind() const noexcept { return kind_; }
    [[nodiscard]] bool is_null() const noexcept { return kind_ == Kind::Null; }
    [[nodiscard]] bool is_bool() const noexcept { return kind_ == Kind::Bool; }
    [[nodiscard]] bool is_number() const noexcept { return kind_ == Kind::Number; }
    [[nodiscard]] bool is_string() const noexcept { return kind_ == Kind::String; }
    [[nodiscard]] bool is_array() const noexcept { return kind_ == Kind::Array; }
    [[nodiscard]] bool is_object() const noexcept { return kind_ == Kind::Object; }

    [[nodiscard]] bool as_bool() const noexcept { return bool_; }
    [[nodiscard]] double as_number() const noexcept { return number_; }
    [[nodiscard]] const std::string& as_string() const noexcept { return string_; }
    [[nodiscard]] const std::vector<JsonValue>& items() const noexcept { return array_; }
    [[nodiscard]] const std::vector<std::pair<std::string, JsonValue>>& members() const noexcept { return object_; }

    void push_back(JsonValue value) { array_.push_back(std::move(value)); }
    void set(std::string key, JsonValue value) { object_.emplace_back(std::move(key), std::move(value)); }

    [[nodiscard]] const JsonValue* find(std::string_view key) const;
    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] std::string dump(bool pretty = false) const;

private:
    void dump_into(std::string& out, bool pretty, int depth) const;

    Kind kind_{Kind::Null};
    bool bool_{false};
    double number_{0.0};
    std::string string_;
    std::vector<JsonValue> array_;
    std::vector<std::pair<std::string, JsonValue>> object_;
};

// Parses a UTF-8 JSON document. Rejects trailing garbage and malformed tokens
// with a byte offset, never throwing.
[[nodiscard]] core::Result<JsonValue> parse_json(std::string_view text);

} // namespace tessera::textio
