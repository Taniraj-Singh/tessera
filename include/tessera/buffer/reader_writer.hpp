#pragma once

#include "tessera/core/types.hpp"

namespace tessera::buffer {

class Reader {
public:
    Reader(core::ByteSpan span) : data_(span.data), remaining_(span.size) {}

    [[nodiscard]] std::size_t remaining() const noexcept { return remaining_; }
    [[nodiscard]] std::size_t position() const noexcept { return position_; }

    core::Result<std::uint8_t> read_u8();
    core::Result<std::uint16_t> read_u16();
    core::Result<std::uint32_t> read_u32();
    core::Result<std::uint64_t> read_u64();
    core::Result<std::vector<std::uint8_t>> read_bytes(std::size_t length);
    core::Result<std::string> read_string(std::size_t length);
    core::Result<core::ByteSpan> read_span(std::size_t length);

    core::Result<void> skip(std::size_t length);

private:
    core::Result<void> require(std::size_t length);

    const std::uint8_t* data_{nullptr};
    std::size_t remaining_{0};
    std::size_t position_{0};
};

class Writer {
public:
    explicit Writer(core::MutableByteSpan span) : data_(span.data), capacity_(span.size) {}

    [[nodiscard]] std::size_t written() const noexcept { return position_; }
    [[nodiscard]] std::size_t remaining() const noexcept { return capacity_ - position_; }

    core::Result<void> write_u8(std::uint8_t value);
    core::Result<void> write_u16(std::uint16_t value);
    core::Result<void> write_u32(std::uint32_t value);
    core::Result<void> write_u64(std::uint64_t value);
    core::Result<void> write_bytes(core::ByteSpan bytes);
    core::Result<void> write_string(std::string_view text);

private:
    core::Result<void> require(std::size_t length);

    std::uint8_t* data_{nullptr};
    std::size_t capacity_{0};
    std::size_t position_{0};
};

} // namespace tessera::buffer
