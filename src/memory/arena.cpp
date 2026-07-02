#include "tessera/memory/arena.hpp"

#include "tessera/util/bit_ops.hpp"

namespace tessera::memory {

namespace {

// Advance a raw address to the next multiple of alignment. Returns the number
// of padding bytes required so the caller can account for them against the
// block's remaining capacity.
std::size_t padding_for(const std::uint8_t* base, std::size_t used, std::size_t alignment) noexcept {
    const auto address = reinterpret_cast<std::uintptr_t>(base + used);
    const std::size_t misaligned = static_cast<std::size_t>(address & (alignment - 1U));
    return misaligned == 0 ? 0 : alignment - misaligned;
}

} // namespace

Arena::Arena(std::size_t block_size)
    : block_size_(block_size < 64U ? std::size_t{64U} : block_size) {}

core::Result<void> Arena::grow(std::size_t minimum) {
    std::size_t capacity = block_size_;
    if (capacity < minimum) {
        capacity = util::align_up(minimum, kMaxAlignment);
    }
    Block block{};
    block.storage = std::make_unique<std::uint8_t[]>(capacity);
    block.capacity = capacity;
    block.used = 0;
    bytes_reserved_ += capacity;
    blocks_.push_back(std::move(block));
    // Grow the default block size gently so long-lived arenas amortize the
    // per-block allocation cost without runaway reservation.
    if (block_size_ < (1U << 20U)) {
        block_size_ <<= 1U;
    }
    return core::Ok();
}

core::Result<void*> Arena::allocate(std::size_t size, std::size_t alignment) {
    if (alignment == 0 || !util::is_power_of_two(alignment)) {
        return core::Error{core::ErrorCode::InvalidLength, "arena alignment must be a power of two", 0};
    }
    if (alignment > kMaxAlignment) {
        alignment = kMaxAlignment;
    }
    if (size == 0) {
        size = 1;
    }

    if (!blocks_.empty()) {
        Block& tail = blocks_.back();
        const std::size_t pad = padding_for(tail.storage.get(), tail.used, alignment);
        if (tail.used + pad + size <= tail.capacity) {
            tail.used += pad;
            void* result = tail.storage.get() + tail.used;
            tail.used += size;
            bytes_allocated_ += size;
            return result;
        }
    }

    // Reserve enough headroom for worst-case alignment padding in the fresh
    // block so the retry below always succeeds.
    if (auto status = grow(size + alignment); !status) {
        return status.error();
    }
    Block& fresh = blocks_.back();
    const std::size_t pad = padding_for(fresh.storage.get(), fresh.used, alignment);
    fresh.used += pad;
    void* result = fresh.storage.get() + fresh.used;
    fresh.used += size;
    bytes_allocated_ += size;
    return result;
}

void Arena::reset() noexcept {
    blocks_.clear();
    bytes_allocated_ = 0;
    bytes_reserved_ = 0;
}

} // namespace tessera::memory
