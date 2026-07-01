#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tessera::core {

enum class ErrorCode : std::uint16_t {
    Ok = 0,
    TruncatedInput,
    InvalidMagic,
    UnsupportedVersion,
    VersionMismatch,
    ChecksumMismatch,
    InvalidLength,
    InvalidEnum,
    InvalidUtf8,
    DuplicateSectionId,
    DuplicateSectionName,
    SectionOutOfBounds,
    SchemaMismatch,
    RecordCountMismatch,
    PluginRejected,
    IoError,
    MapFailed,
    InternalError,
};

struct Error {
    ErrorCode code{ErrorCode::InternalError};
    std::string message;
    std::size_t offset{0};

    [[nodiscard]] bool ok() const noexcept { return code == ErrorCode::Ok; }
};

template <typename T>
class Result {
public:
    Result(T value) : storage_(std::move(value)) {} // NOLINT(google-explicit-constructor)

    Result(Error error) : storage_(std::move(error)) {} // NOLINT(google-explicit-constructor)

    [[nodiscard]] bool ok() const noexcept { return std::holds_alternative<T>(storage_); }
    [[nodiscard]] explicit operator bool() const noexcept { return ok(); }

    [[nodiscard]] T& value() & { return std::get<T>(storage_); }
    [[nodiscard]] const T& value() const& { return std::get<T>(storage_); }
    [[nodiscard]] T&& value() && { return std::get<T>(std::move(storage_)); }

    [[nodiscard]] T& operator*() & { return value(); }
    [[nodiscard]] const T& operator*() const& { return value(); }
    [[nodiscard]] T&& operator*() && { return std::move(value()); }

    [[nodiscard]] T* operator->() { return &value(); }
    [[nodiscard]] const T* operator->() const { return &value(); }

    [[nodiscard]] Error& error() & { return std::get<Error>(storage_); }
    [[nodiscard]] const Error& error() const& { return std::get<Error>(storage_); }

private:
    std::variant<T, Error> storage_;
};

template <>
class Result<void> {
public:
    Result(bool ok = true) : ok_(ok) {} // NOLINT(google-explicit-constructor)
    Result(Error error) : ok_(false), error_(std::move(error)) {}

    [[nodiscard]] bool ok() const noexcept { return ok_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ok_; }

    [[nodiscard]] const Error& error() const& { return error_; }

private:
    bool ok_{true};
    Error error_{};
};

inline Result<void> Ok() { return Result<void>(true); }

struct ByteSpan {
    const std::uint8_t* data{nullptr};
    std::size_t size{0};

    [[nodiscard]] const std::uint8_t* begin() const noexcept { return data; }
    [[nodiscard]] const std::uint8_t* end() const noexcept { return data + size; }
    [[nodiscard]] bool empty() const noexcept { return size == 0; }
};

struct MutableByteSpan {
    std::uint8_t* data{nullptr};
    std::size_t size{0};

    [[nodiscard]] std::uint8_t* begin() noexcept { return data; }
    [[nodiscard]] std::uint8_t* end() noexcept { return data + size; }
    [[nodiscard]] bool empty() const noexcept { return size == 0; }
};

[[nodiscard]] std::uint16_t read_le_u16(const std::uint8_t* bytes) noexcept;
[[nodiscard]] std::uint32_t read_le_u32(const std::uint8_t* bytes) noexcept;
[[nodiscard]] std::uint64_t read_le_u64(const std::uint8_t* bytes) noexcept;

void write_le_u16(std::uint8_t* bytes, std::uint16_t value) noexcept;
void write_le_u32(std::uint8_t* bytes, std::uint32_t value) noexcept;
void write_le_u64(std::uint8_t* bytes, std::uint64_t value) noexcept;

[[nodiscard]] bool is_valid_utf8(std::string_view text) noexcept;

} // namespace tessera::core
