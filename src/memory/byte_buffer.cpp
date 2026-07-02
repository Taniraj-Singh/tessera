#include "tessera/memory/byte_buffer.hpp"

namespace tessera::memory {

ByteBuffer::ByteBuffer(std::size_t initial_capacity) {
    storage_.reserve(initial_capacity);
}

void ByteBuffer::append_u8(std::uint8_t value) {
    storage_.push_back(value);
}

void ByteBuffer::append_u16(std::uint16_t value) {
    const std::size_t offset = storage_.size();
    storage_.resize(offset + 2);
    core::write_le_u16(storage_.data() + offset, value);
}

void ByteBuffer::append_u32(std::uint32_t value) {
    const std::size_t offset = storage_.size();
    storage_.resize(offset + 4);
    core::write_le_u32(storage_.data() + offset, value);
}

void ByteBuffer::append_u64(std::uint64_t value) {
    const std::size_t offset = storage_.size();
    storage_.resize(offset + 8);
    core::write_le_u64(storage_.data() + offset, value);
}

void ByteBuffer::append_bytes(core::ByteSpan bytes) {
    storage_.insert(storage_.end(), bytes.begin(), bytes.end());
}

void ByteBuffer::append_string(std::string_view text) {
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(text.data());
    storage_.insert(storage_.end(), bytes, bytes + text.size());
}

core::Result<void> ByteBuffer::patch_u32(std::size_t offset, std::uint32_t value) {
    if (offset + 4 > storage_.size()) {
        return core::Error{core::ErrorCode::InvalidLength, "patch_u32 out of range", offset};
    }
    core::write_le_u32(storage_.data() + offset, value);
    return core::Ok();
}

} // namespace tessera::memory
