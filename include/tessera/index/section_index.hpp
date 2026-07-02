#pragma once

#include "tessera/index/hash_index.hpp"
#include "tessera/index/string_index.hpp"
#include "tessera/model/container.hpp"

#include <cstdint>
#include <string_view>

namespace tessera::index {

// Dual lookup over a container's sections: by numeric id and by name. Both map
// to the section's slot in Container::sections, so callers resolve a section in
// O(1) instead of the linear scans the validator and converter used before.
//
// The index is a snapshot built from a container; it does not observe later
// mutations. Rebuild after structural changes.
class SectionIndex {
public:
    SectionIndex() = default;

    [[nodiscard]] static SectionIndex build(const model::Container& container);

    // Returns a pointer to the section slot for the id/name, or nullptr.
    [[nodiscard]] const std::uint32_t* find_by_id(std::uint16_t id) const;
    [[nodiscard]] const std::uint32_t* find_by_name(std::string_view name) const;

    [[nodiscard]] std::size_t size() const noexcept { return count_; }

private:
    HashIndex by_id_;
    StringIndex by_name_;
    std::size_t count_{0};
};

} // namespace tessera::index
