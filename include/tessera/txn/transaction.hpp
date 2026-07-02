#pragma once

#include "tessera/core/types.hpp"
#include "tessera/model/container.hpp"

#include <cstdint>
#include <string>

namespace tessera::txn {

// Stages edits to a container and applies them atomically. Operations mutate a
// private working copy; commit() validates the result and returns the new
// container only if it is semantically sound, while rollback() discards the
// staged edits and restores the original snapshot. The caller's base container
// is never modified in place.
class Transaction {
public:
    explicit Transaction(model::Container base);

    [[nodiscard]] core::Result<void> add_section(model::SectionData section);
    [[nodiscard]] core::Result<void> remove_section(std::uint16_t id);
    [[nodiscard]] core::Result<void> rename_section(std::uint16_t id, std::string new_name);

    void rollback();

    // Runs semantic validation over the staged container; on success returns the
    // edited container, otherwise the first validation error.
    [[nodiscard]] core::Result<model::Container> commit() const;

    [[nodiscard]] std::size_t pending_ops() const noexcept { return pending_ops_; }
    [[nodiscard]] std::size_t section_count() const noexcept { return working_.sections.size(); }

private:
    void resync_header();
    [[nodiscard]] std::size_t find_slot(std::uint16_t id) const;
    static constexpr std::size_t kNotFound = static_cast<std::size_t>(-1);

    model::Container base_;
    model::Container working_;
    std::size_t pending_ops_{0};
};

} // namespace tessera::txn
