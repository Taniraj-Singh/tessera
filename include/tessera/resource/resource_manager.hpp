#pragma once

#include "tessera/core/types.hpp"

#include <cstddef>

namespace tessera::resource {

struct Budget {
    std::size_t max_bytes{0};   // 0 means unbounded
    std::size_t max_handles{0}; // 0 means unbounded
};

// Tracks memory and handle usage against a budget. Acquisition returns an error
// when a request would exceed either limit, letting a subsystem bound its
// footprint. Not thread-safe.
class ResourceManager {
public:
    explicit ResourceManager(Budget budget) : budget_(budget) {}

    [[nodiscard]] core::Result<void> acquire(std::size_t bytes);
    void release(std::size_t bytes) noexcept;

    [[nodiscard]] std::size_t used_bytes() const noexcept { return used_bytes_; }
    [[nodiscard]] std::size_t handle_count() const noexcept { return handles_; }
    [[nodiscard]] const Budget& budget() const noexcept { return budget_; }
    [[nodiscard]] bool within_budget() const noexcept;

private:
    Budget budget_;
    std::size_t used_bytes_{0};
    std::size_t handles_{0};
};

// RAII reservation. Acquires on creation via take(); returns the bytes to the
// manager on destruction. Move-only so a reservation is released exactly once.
class Lease {
public:
    [[nodiscard]] static core::Result<Lease> take(ResourceManager& manager, std::size_t bytes);

    Lease(const Lease&) = delete;
    Lease& operator=(const Lease&) = delete;
    Lease(Lease&& other) noexcept;
    Lease& operator=(Lease&& other) noexcept;
    ~Lease();

    [[nodiscard]] std::size_t bytes() const noexcept { return bytes_; }

private:
    Lease(ResourceManager& manager, std::size_t bytes) : manager_(&manager), bytes_(bytes) {}
    void release() noexcept;

    ResourceManager* manager_{nullptr};
    std::size_t bytes_{0};
};

} // namespace tessera::resource
