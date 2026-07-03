#include "tessera/mmap/mapped_file.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace tessera::mmap {

MappedFile::~MappedFile() {
    release();
}

MappedFile::MappedFile(MappedFile&& other) noexcept {
    view_ = other.view_;
#if defined(_WIN32)
    file_handle_ = other.file_handle_;
    mapping_handle_ = other.mapping_handle_;
    other.file_handle_ = nullptr;
    other.mapping_handle_ = nullptr;
#else
    mapped_ = other.mapped_;
    fd_ = other.fd_;
    other.mapped_ = nullptr;
    other.fd_ = -1;
#endif
    other.view_ = {};
}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept {
    if (this != &other) {
        release();
        view_ = other.view_;
#if defined(_WIN32)
        file_handle_ = other.file_handle_;
        mapping_handle_ = other.mapping_handle_;
        other.file_handle_ = nullptr;
        other.mapping_handle_ = nullptr;
#else
        mapped_ = other.mapped_;
        fd_ = other.fd_;
        other.mapped_ = nullptr;
        other.fd_ = -1;
#endif
        other.view_ = {};
    }
    return *this;
}

void MappedFile::release() noexcept {
#if defined(_WIN32)
    if (view_.data != nullptr) {
        UnmapViewOfFile(view_.data);
    }
    if (mapping_handle_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(mapping_handle_));
    }
    if (file_handle_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(file_handle_));
    }
    mapping_handle_ = nullptr;
    file_handle_ = nullptr;
#else
    if (mapped_ != nullptr && mapped_ != MAP_FAILED) {
        munmap(mapped_, view_.size);
    }
    if (fd_ >= 0) {
        close(fd_);
    }
    mapped_ = nullptr;
    fd_ = -1;
#endif
    view_ = {};
}

core::Result<MappedFile> MappedFile::open(const std::string& path) {
    MappedFile mapped{};

#if defined(_WIN32)
    HANDLE file = CreateFileA(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return core::Error{core::ErrorCode::IoError, "failed to open file for mapping", 0};
    }

    LARGE_INTEGER file_size{};
    if (!GetFileSizeEx(file, &file_size) || file_size.QuadPart < 0) {
        CloseHandle(file);
        return core::Error{core::ErrorCode::IoError, "failed to query file size", 0};
    }

    if (file_size.QuadPart == 0) {
        CloseHandle(file);
        mapped.view_ = core::ByteSpan{};
        return mapped;
    }

    HANDLE mapping = CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (mapping == nullptr) {
        CloseHandle(file);
        return core::Error{core::ErrorCode::MapFailed, "CreateFileMapping failed", 0};
    }

    const auto* view = static_cast<const std::uint8_t*>(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0));
    if (view == nullptr) {
        CloseHandle(mapping);
        CloseHandle(file);
        return core::Error{core::ErrorCode::MapFailed, "MapViewOfFile failed", 0};
    }

    mapped.file_handle_ = file;
    mapped.mapping_handle_ = mapping;
    mapped.view_ = core::ByteSpan{view, static_cast<std::size_t>(file_size.QuadPart)};
    return mapped;
#else
    const int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return core::Error{core::ErrorCode::IoError, "failed to open file for mapping", 0};
    }

    struct stat stat_buf {};
    if (fstat(fd, &stat_buf) != 0) {
        close(fd);
        return core::Error{core::ErrorCode::IoError, "failed to stat mapped file", 0};
    }

    if (stat_buf.st_size == 0) {
        close(fd);
        mapped.view_ = core::ByteSpan{};
        return mapped;
    }

    void* mapped_ptr = ::mmap(nullptr, static_cast<std::size_t>(stat_buf.st_size), PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_ptr == MAP_FAILED) {
        close(fd);
        return core::Error{core::ErrorCode::MapFailed, "mmap failed", 0};
    }

    mapped.fd_ = fd;
    mapped.mapped_ = mapped_ptr;
    mapped.view_ = core::ByteSpan{
        static_cast<const std::uint8_t*>(mapped_ptr),
        static_cast<std::size_t>(stat_buf.st_size),
    };
    return mapped;
#endif
}

} // namespace tessera::mmap
