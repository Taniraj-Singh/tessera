#pragma once

#include "tessera/core/types.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>

namespace tessera::config {

// Simple line-oriented configuration store. Text is `key = value` per line, with
// `#` comments and blank lines ignored. Typed getters coerce the stored string
// and fall back to a default when the key is absent or unparseable, so callers
// never have to guard every lookup.
class Config {
public:
    Config() = default;

    // Parses configuration text. A non-comment, non-blank line without '=' is a
    // structural error reported with its line offset.
    [[nodiscard]] static core::Result<Config> parse(std::string_view text);

    void set(std::string key, std::string value);
    [[nodiscard]] bool has(std::string_view key) const;
    [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }

    [[nodiscard]] std::string get_string(std::string_view key, std::string_view fallback = "") const;
    [[nodiscard]] std::int64_t get_int(std::string_view key, std::int64_t fallback = 0) const;
    [[nodiscard]] bool get_bool(std::string_view key, bool fallback = false) const;
    [[nodiscard]] double get_double(std::string_view key, double fallback = 0.0) const;

private:
    [[nodiscard]] const std::string* lookup(std::string_view key) const;

    std::map<std::string, std::string, std::less<>> values_;
};

} // namespace tessera::config
