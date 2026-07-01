#include "tessera/buffer/reader_writer.hpp"

#include "tessera/core/constants.hpp"

namespace tessera::buffer {

core::Result<void> Reader::require(std::size_t length) {
    if (length > remaining_) {
        return core::Error{core::ErrorCode::TruncatedInput, "unexpected end of input", position_};
    }
    return core::Ok();
}

core::Result<std::uint8_t> Reader::read_u8() {
    if (auto check = require(1); !check) {
        return check.error();
    }
    const std::uint8_t value = data_[0];
    data_ += 1;
    remaining_ -= 1;
    position_ += 1;
    return value;
}

core::Result<std::uint16_t> Reader::read_u16() {
    if (auto check = require(2); !check) {
        return check.error();
    }
    const std::uint16_t value = core::read_le_u16(data_);
    data_ += 2;
    remaining_ -= 2;
    position_ += 2;
    return value;
}

core::Result<std::uint32_t> Reader::read_u32() {
    if (auto check = require(4); !check) {
        return check.error();
    }
    const std::uint32_t value = core::read_le_u32(data_);
    data_ += 4;
    remaining_ -= 4;
    position_ += 4;
    return value;
}

core::Result<std::uint64_t> Reader::read_u64() {
    if (auto check = require(8); !check) {
        return check.error();
    }
    const std::uint64_t value = core::read_le_u64(data_);
    data_ += 8;
    remaining_ -= 8;
    position_ += 8;
    return value;
}

core::Result<std::vector<std::uint8_t>> Reader::read_bytes(std::size_t length) {
    if (length > core::kMaxBlockSize) {
        return core::Error{core::ErrorCode::InvalidLength, "requested byte length exceeds limit", position_};
    }
    if (auto check = require(length); !check) {
        return check.error();
    }
    std::vector<std::uint8_t> output(length);
    for (std::size_t index = 0; index < length; ++index) {
        output[index] = data_[index];
    }
    data_ += length;
    remaining_ -= length;
    position_ += length;
    return output;
}

core::Result<std::string> Reader::read_string(std::size_t length) {
    auto bytes = read_bytes(length);
    if (!bytes) {
        return bytes.error();
    }
    std::string text(reinterpret_cast<const char*>(bytes.value().data()), bytes.value().size());
    if (!core::is_valid_utf8(text)) {
        return core::Error{core::ErrorCode::InvalidUtf8, "invalid UTF-8 sequence", position_ - length};
    }
    return text;
}

core::Result<core::ByteSpan> Reader::read_span(std::size_t length) {
    if (length > core::kMaxBlockSize) {
        return core::Error{core::ErrorCode::InvalidLength, "requested span length exceeds limit", position_};
    }
    if (auto check = require(length); !check) {
        return check.error();
    }
    core::ByteSpan span{data_, length};
    data_ += length;
    remaining_ -= length;
    position_ += length;
    return span;
}

core::Result<void> Reader::skip(std::size_t length) {
    if (auto check = require(length); !check) {
        return check.error();
    }
    data_ += length;
    remaining_ -= length;
    position_ += length;
    return core::Ok();
}

core::Result<void> Writer::require(std::size_t length) {
    if (position_ + length > capacity_) {
        return core::Error{core::ErrorCode::InvalidLength, "writer buffer overflow", position_};
    }
    return core::Ok();
}

core::Result<void> Writer::write_u8(std::uint8_t value) {
    if (auto check = require(1); !check) {
        return check.error();
    }
    data_[position_] = value;
    position_ += 1;
    return core::Ok();
}

core::Result<void> Writer::write_u16(std::uint16_t value) {
    if (auto check = require(2); !check) {
        return check.error();
    }
    core::write_le_u16(data_ + position_, value);
    position_ += 2;
    return core::Ok();
}

core::Result<void> Writer::write_u32(std::uint32_t value) {
    if (auto check = require(4); !check) {
        return check.error();
    }
    core::write_le_u32(data_ + position_, value);
    position_ += 4;
    return core::Ok();
}

core::Result<void> Writer::write_u64(std::uint64_t value) {
    if (auto check = require(8); !check) {
        return check.error();
    }
    core::write_le_u64(data_ + position_, value);
    position_ += 8;
    return core::Ok();
}

core::Result<void> Writer::write_bytes(core::ByteSpan bytes) {
    if (auto check = require(bytes.size); !check) {
        return check.error();
    }
    for (std::size_t index = 0; index < bytes.size; ++index) {
        data_[position_ + index] = bytes.data[index];
    }
    position_ += bytes.size;
    return core::Ok();
}

core::Result<void> Writer::write_string(std::string_view text) {
    if (!core::is_valid_utf8(text)) {
        return core::Error{core::ErrorCode::InvalidUtf8, "invalid UTF-8 in write_string", position_};
    }
    return write_bytes(core::ByteSpan{reinterpret_cast<const std::uint8_t*>(text.data()), text.size()});
}

} // namespace tessera::buffer
