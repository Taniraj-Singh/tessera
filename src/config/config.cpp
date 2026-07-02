#include "tessera/config/config.hpp"

#include <cstdlib>

namespace tessera::config {

namespace {

std::string_view trim(std::string_view text) noexcept {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && (text[begin] == ' ' || text[begin] == '\t')) {
        ++begin;
    }
    while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r')) {
        --end;
    }
    return text.substr(begin, end - begin);
}

} // namespace

core::Result<Config> Config::parse(std::string_view text) {
    Config config{};
    std::size_t line_start = 0;
    while (line_start <= text.size()) {
        std::size_t newline = text.find('\n', line_start);
        const std::size_t line_end = newline == std::string_view::npos ? text.size() : newline;
        const std::string_view raw = text.substr(line_start, line_end - line_start);
        const std::string_view line = trim(raw);

        if (!line.empty() && line.front() != '#') {
            const std::size_t equals = line.find('=');
            if (equals == std::string_view::npos) {
                return core::Error{core::ErrorCode::InvalidEnum, "malformed config line (no '=')", line_start};
            }
            const std::string_view key = trim(line.substr(0, equals));
            const std::string_view value = trim(line.substr(equals + 1));
            if (key.empty()) {
                return core::Error{core::ErrorCode::InvalidEnum, "config line with empty key", line_start};
            }
            config.set(std::string(key), std::string(value));
        }

        if (newline == std::string_view::npos) {
            break;
        }
        line_start = line_end + 1;
    }
    return config;
}

void Config::set(std::string key, std::string value) {
    values_[std::move(key)] = std::move(value);
}

const std::string* Config::lookup(std::string_view key) const {
    auto it = values_.find(key);
    return it == values_.end() ? nullptr : &it->second;
}

bool Config::has(std::string_view key) const {
    return lookup(key) != nullptr;
}

std::string Config::get_string(std::string_view key, std::string_view fallback) const {
    const std::string* value = lookup(key);
    return value != nullptr ? *value : std::string(fallback);
}

std::int64_t Config::get_int(std::string_view key, std::int64_t fallback) const {
    const std::string* value = lookup(key);
    if (value == nullptr || value->empty()) {
        return fallback;
    }
    char* end = nullptr;
    const long long parsed = std::strtoll(value->c_str(), &end, 10);
    if (end == value->c_str() || *end != '\0') {
        return fallback;
    }
    return static_cast<std::int64_t>(parsed);
}

bool Config::get_bool(std::string_view key, bool fallback) const {
    const std::string* value = lookup(key);
    if (value == nullptr) {
        return fallback;
    }
    if (*value == "true" || *value == "1" || *value == "yes" || *value == "on") {
        return true;
    }
    if (*value == "false" || *value == "0" || *value == "no" || *value == "off") {
        return false;
    }
    return fallback;
}

double Config::get_double(std::string_view key, double fallback) const {
    const std::string* value = lookup(key);
    if (value == nullptr || value->empty()) {
        return fallback;
    }
    char* end = nullptr;
    const double parsed = std::strtod(value->c_str(), &end);
    if (end == value->c_str() || *end != '\0') {
        return fallback;
    }
    return parsed;
}

} // namespace tessera::config
