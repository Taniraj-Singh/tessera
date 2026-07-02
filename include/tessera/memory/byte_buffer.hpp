#pragma once

#include "tessera/core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace tessera::memory {

// Owning, growable byte sink used by the encoder-side subsystems (compression
// frames, archive writer, incremental encoder). It complements
// buffer::Writer — Writer performs bounded writes into borrowed memory, whereas
// ByteBuffer owns its storage and grows geometrically. Little-endian append
// helpers mirror the core::write_le_* conventions so serialized layouts stay
// byte-identical regardless of which writer produced them.
class ByteBuffer {
public:
    ByteBuffer() = default;
    explicit ByteBuffer(std::size_t initial_capacity);

    [[nodiscard]] std::size_t size() const noexcept { return storage_.size(); }
    [[nodiscard]] bool empty() const noexcept { return storage_.empty(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return storage_.capacity(); }

    [[nodiscard]] std::uint8_t* data() noexcept { return storage_.data(); }
    [[nodiscard]] const std::uint8_t* data() const noexcept { return storage_.data(); }

    [[nodiscard]] core::ByteSpan span() const noexcept { return core::ByteSpan{storage_.data(), storage_.size()}; }
    [[nodiscard]] core::MutableByteSpan mutable_span() noexcept {
        return core::MutableByteSpan{storage_.data(), storage_.size()};
    }

    void clear() noexcept { storage_.clear(); }
    void reserve(std::size_t capacity) { storage_.reserve(capacity); }
    void resize(std::size_t size) { storage_.resize(size); }

    void append_u8(std::uint8_t value);
    void append_u16(std::uint16_t value);
    void append_u32(std::uint32_t value);
    void append_u64(std::uint64_t value);
    void append_bytes(core::ByteSpan bytes);
    void append_string(std::string_view text);

    // Overwrites size() bytes at the given offset without growing the buffer.
    // Used to backfill length prefixes once a block's extent is known. The
    // caller is responsible for offset + span.size <= size().
    [[nodiscard]] core::Result<void> patch_u32(std::size_t offset, std::uint32_t value);

    [[nodiscard]] std::vector<std::uint8_t> take() noexcept { return std::move(storage_); }
    [[nodiscard]] const std::vector<std::uint8_t>& raw() const noexcept { return storage_; }

private:
    std::vector<std::uint8_t> storage_;
};

} // namespace tessera::memory
