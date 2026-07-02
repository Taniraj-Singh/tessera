#pragma once

#include "tessera/core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tessera::index {

// Open-addressing map from a string key to a 32-bit value. Unlike HashIndex it
// stores the full key so distinct strings that collide under the hash are
// resolved by comparison rather than silently aliased. Used to look up sections
// (and later, schema fields) by name.
class StringIndex {
public:
    static constexpr std::size_t kMinCapacity = 8;
    static constexpr std::size_t kLoadNumerator = 11;
    static constexpr std::size_t kLoadDenominator = 16;

    StringIndex() : StringIndex(kMinCapacity) {}
    explicit StringIndex(std::size_t initial_capacity);

    bool insert(std::string_view key, std::uint32_t value);

    [[nodiscard]] const std::uint32_t* find(std::string_view key) const;
    [[nodiscard]] bool contains(std::string_view key) const { return find(key) != nullptr; }

    [[nodiscard]] std::size_t size() const noexcept { return live_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return slots_.size(); }
    [[nodiscard]] bool empty() const noexcept { return live_ == 0; }

    void clear();

private:
    struct Slot {
        std::string key;
        std::uint32_t value{0};
        bool occupied{false};
    };

    [[nodiscard]] std::size_t probe_start(std::string_view key) const noexcept;
    void rehash(std::size_t new_capacity);

    std::vector<Slot> slots_;
    std::size_t live_{0};
};

} // namespace tessera::index
