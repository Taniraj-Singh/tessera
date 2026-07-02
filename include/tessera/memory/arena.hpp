#pragma once

#include "tessera/core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace tessera::memory {

// Monotonic (bump-pointer) region allocator. Hands out aligned, uninitialized
// storage from a chain of geometrically growing blocks and frees everything at
// once on reset()/destruction. It is the backing store for the transient node
// graphs built by the object-graph and pipeline subsystems, where thousands of
// small nodes share a single lifetime and per-node delete would dominate.
//
// The arena never relocates a live allocation, so pointers it returns stay
// valid until reset(). It is not thread-safe; callers scope one arena per
// worker.
class Arena {
public:
    static constexpr std::size_t kDefaultBlockSize = 64U * 1024U;
    static constexpr std::size_t kMaxAlignment = alignof(std::max_align_t);

    Arena() : Arena(kDefaultBlockSize) {}
    explicit Arena(std::size_t block_size);

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) noexcept = default;
    Arena& operator=(Arena&&) noexcept = default;

    // Returns a pointer to size bytes aligned to alignment, or an error if the
    // request is degenerate (alignment not a power of two, or size so large it
    // would overflow the block bookkeeping).
    [[nodiscard]] core::Result<void*> allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t));

    template <typename T, typename... Args>
    [[nodiscard]] core::Result<T*> create(Args&&... args) {
        auto storage = allocate(sizeof(T), alignof(T));
        if (!storage) {
            return storage.error();
        }
        return new (storage.value()) T(std::forward<Args>(args)...);
    }

    void reset() noexcept;

    [[nodiscard]] std::size_t bytes_allocated() const noexcept { return bytes_allocated_; }
    [[nodiscard]] std::size_t bytes_reserved() const noexcept { return bytes_reserved_; }
    [[nodiscard]] std::size_t block_count() const noexcept { return blocks_.size(); }

private:
    struct Block {
        std::unique_ptr<std::uint8_t[]> storage;
        std::size_t capacity{0};
        std::size_t used{0};
    };

    [[nodiscard]] core::Result<void> grow(std::size_t minimum);

    std::vector<Block> blocks_;
    std::size_t block_size_{kDefaultBlockSize};
    std::size_t bytes_allocated_{0};
    std::size_t bytes_reserved_{0};
};

} // namespace tessera::memory
