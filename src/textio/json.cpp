#include "tessera/textio/json.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace tessera::textio {

const JsonValue* JsonValue::find(std::string_view key) const {
    for (const auto& member : object_) {
        if (member.first == key) {
            return &member.second;
        }
    }
    return nullptr;
}

std::size_t JsonValue::size() const noexcept {
    if (kind_ == Kind::Array) {
        return array_.size();
    }
    if (kind_ == Kind::Object) {
        return object_.size();
    }
    return 0;
}

namespace {

void append_escaped(std::string& out, const std::string& text) {
    out.push_back('"');
    for (char character : text) {
        const auto byte = static_cast<unsigned char>(character);
        switch (character) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        default:
            if (byte < 0x20U) {
                static const char* kHex = "0123456789abcdef";
                out += "\\u00";
                out.push_back(kHex[(byte >> 4U) & 0xFU]);
                out.push_back(kHex[byte & 0xFU]);
            } else {
                out.push_back(character);
            }
            break;
        }
    }
    out.push_back('"');
}

void append_number(std::string& out, double value) {
    // Integers render without a trailing ".0"; everything else uses %g precision.
    const long long as_integer = static_cast<long long>(value);
    if (static_cast<double>(as_integer) == value) {
        out += std::to_string(as_integer);
        return;
    }
    std::array<char, 64> buffer{};
    const int written = std::snprintf(buffer.data(), buffer.size(), "%.17g", value);
    if (written > 0 && static_cast<std::size_t>(written) < buffer.size()) {
        out.append(buffer.data(), static_cast<std::size_t>(written));
    } else {
        out += "0";
    }
}

void indent(std::string& out, int depth) {
    for (int level = 0; level < depth; ++level) {
        out += "  ";
    }
}

} // namespace

void JsonValue::dump_into(std::string& out, bool pretty, int depth) const {
    switch (kind_) {
    case Kind::Null:
        out += "null";
        break;
    case Kind::Bool:
        out += bool_ ? "true" : "false";
        break;
    case Kind::Number:
        append_number(out, number_);
        break;
    case Kind::String:
        append_escaped(out, string_);
        break;
    case Kind::Array: {
        if (array_.empty()) {
            out += "[]";
            break;
        }
        out.push_back('[');
        for (std::size_t index = 0; index < array_.size(); ++index) {
            if (index != 0) {
                out.push_back(',');
            }
            if (pretty) {
                out.push_back('\n');
                indent(out, depth + 1);
            }
            array_[index].dump_into(out, pretty, depth + 1);
        }
        if (pretty) {
            out.push_back('\n');
            indent(out, depth);
        }
        out.push_back(']');
        break;
    }
    case Kind::Object: {
        if (object_.empty()) {
            out += "{}";
            break;
        }
        out.push_back('{');
        for (std::size_t index = 0; index < object_.size(); ++index) {
            if (index != 0) {
                out.push_back(',');
            }
            if (pretty) {
                out.push_back('\n');
                indent(out, depth + 1);
            }
            append_escaped(out, object_[index].first);
            out.push_back(':');
            if (pretty) {
                out.push_back(' ');
            }
            object_[index].second.dump_into(out, pretty, depth + 1);
        }
        if (pretty) {
            out.push_back('\n');
            indent(out, depth);
        }
        out.push_back('}');
        break;
    }
    }
}

std::string JsonValue::dump(bool pretty) const {
    std::string out;
    dump_into(out, pretty, 0);
    return out;
}

namespace {

class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {}

    core::Result<JsonValue> parse() {
        skip_whitespace();
        auto value = parse_value();
        if (!value) {
            return value.error();
        }
        skip_whitespace();
        if (pos_ != text_.size()) {
            return core::Error{core::ErrorCode::InvalidLength, "trailing data after json value", pos_};
        }
        return value;
    }

private:
    void skip_whitespace() {
        while (pos_ < text_.size()) {
            const char character = text_[pos_];
            if (character == ' ' || character == '\t' || character == '\n' || character == '\r') {
                ++pos_;
            } else {
                break;
            }
        }
    }

    [[nodiscard]] bool consume(char expected) {
        if (pos_ < text_.size() && text_[pos_] == expected) {
            ++pos_;
            return true;
        }
        return false;
    }

    [[nodiscard]] bool match_literal(std::string_view literal) {
        if (text_.substr(pos_, literal.size()) == literal) {
            pos_ += literal.size();
            return true;
        }
        return false;
    }

    core::Result<JsonValue> parse_value() {
        if (pos_ >= text_.size()) {
            return core::Error{core::ErrorCode::TruncatedInput, "unexpected end of json", pos_};
        }
        const char character = text_[pos_];
        switch (character) {
        case '{':
            return parse_object();
        case '[':
            return parse_array();
        case '"': {
            auto string = parse_string();
            if (!string) {
                return string.error();
            }
            return JsonValue{std::move(*string)};
        }
        case 't':
            if (match_literal("true")) {
                return JsonValue{true};
            }
            return core::Error{core::ErrorCode::InvalidEnum, "invalid json literal", pos_};
        case 'f':
            if (match_literal("false")) {
                return JsonValue{false};
            }
            return core::Error{core::ErrorCode::InvalidEnum, "invalid json literal", pos_};
        case 'n':
            if (match_literal("null")) {
                return JsonValue{};
            }
            return core::Error{core::ErrorCode::InvalidEnum, "invalid json literal", pos_};
        default:
            return parse_number();
        }
    }

    core::Result<JsonValue> parse_object() {
        ++pos_; // consume '{'
        JsonValue object = JsonValue::object();
        skip_whitespace();
        if (consume('}')) {
            return object;
        }
        for (;;) {
            skip_whitespace();
            if (pos_ >= text_.size() || text_[pos_] != '"') {
                return core::Error{core::ErrorCode::InvalidEnum, "expected object key", pos_};
            }
            auto key = parse_string();
            if (!key) {
                return key.error();
            }
            skip_whitespace();
            if (!consume(':')) {
                return core::Error{core::ErrorCode::InvalidEnum, "expected ':' in object", pos_};
            }
            skip_whitespace();
            auto value = parse_value();
            if (!value) {
                return value.error();
            }
            object.set(std::move(*key), std::move(*value));
            skip_whitespace();
            if (consume(',')) {
                continue;
            }
            if (consume('}')) {
                return object;
            }
            return core::Error{core::ErrorCode::InvalidEnum, "expected ',' or '}' in object", pos_};
        }
    }

    core::Result<JsonValue> parse_array() {
        ++pos_; // consume '['
        JsonValue array = JsonValue::array();
        skip_whitespace();
        if (consume(']')) {
            return array;
        }
        for (;;) {
            skip_whitespace();
            auto value = parse_value();
            if (!value) {
                return value.error();
            }
            array.push_back(std::move(*value));
            skip_whitespace();
            if (consume(',')) {
                continue;
            }
            if (consume(']')) {
                return array;
            }
            return core::Error{core::ErrorCode::InvalidEnum, "expected ',' or ']' in array", pos_};
        }
    }

    core::Result<std::string> parse_string() {
        ++pos_; // consume opening quote
        std::string out;
        while (pos_ < text_.size()) {
            const char character = text_[pos_++];
            if (character == '"') {
                return out;
            }
            if (character != '\\') {
                out.push_back(character);
                continue;
            }
            if (pos_ >= text_.size()) {
                break;
            }
            const char escape = text_[pos_++];
            switch (escape) {
            case '"':
                out.push_back('"');
                break;
            case '\\':
                out.push_back('\\');
                break;
            case '/':
                out.push_back('/');
                break;
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            case 'b':
                out.push_back('\b');
                break;
            case 'f':
                out.push_back('\f');
                break;
            case 'u': {
                if (auto status = parse_unicode_escape(out); !status) {
                    return status.error();
                }
                break;
            }
            default:
                return core::Error{core::ErrorCode::InvalidUtf8, "invalid string escape", pos_};
            }
        }
        return core::Error{core::ErrorCode::TruncatedInput, "unterminated json string", pos_};
    }

    core::Result<void> parse_unicode_escape(std::string& out) {
        if (pos_ + 4 > text_.size()) {
            return core::Error{core::ErrorCode::TruncatedInput, "truncated unicode escape", pos_};
        }
        std::uint32_t code = 0;
        for (int digit = 0; digit < 4; ++digit) {
            const char hex = text_[pos_++];
            code <<= 4U;
            if (hex >= '0' && hex <= '9') {
                code |= static_cast<std::uint32_t>(hex - '0');
            } else if (hex >= 'a' && hex <= 'f') {
                code |= static_cast<std::uint32_t>(hex - 'a' + 10);
            } else if (hex >= 'A' && hex <= 'F') {
                code |= static_cast<std::uint32_t>(hex - 'A' + 10);
            } else {
                return core::Error{core::ErrorCode::InvalidUtf8, "invalid unicode escape digit", pos_};
            }
        }
        // Encode the basic-multilingual-plane code point as UTF-8.
        if (code < 0x80U) {
            out.push_back(static_cast<char>(code));
        } else if (code < 0x800U) {
            out.push_back(static_cast<char>(0xC0U | (code >> 6U)));
            out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
        } else {
            out.push_back(static_cast<char>(0xE0U | (code >> 12U)));
            out.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
            out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
        }
        return core::Ok();
    }

    core::Result<JsonValue> parse_number() {
        const std::size_t start = pos_;
        if (consume('-')) {
            // sign consumed
        }
        while (pos_ < text_.size()) {
            const char character = text_[pos_];
            const bool numeric = (character >= '0' && character <= '9') || character == '.' || character == 'e' ||
                                  character == 'E' || character == '+' || character == '-';
            if (!numeric) {
                break;
            }
            ++pos_;
        }
        if (pos_ == start) {
            return core::Error{core::ErrorCode::InvalidEnum, "invalid json value", pos_};
        }
        const std::string token(text_.substr(start, pos_ - start));
        char* end = nullptr;
        const double value = std::strtod(token.c_str(), &end);
        if (end != token.c_str() + token.size()) {
            return core::Error{core::ErrorCode::InvalidLength, "malformed json number", start};
        }
        return JsonValue{value};
    }

    std::string_view text_;
    std::size_t pos_{0};
};

} // namespace

core::Result<JsonValue> parse_json(std::string_view text) {
    Parser parser(text);
    return parser.parse();
}

} // namespace tessera::textio
