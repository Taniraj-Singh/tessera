#pragma once

#include "tessera/core/types.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace tessera::mmap {

class MappedFile {
public:
    MappedFile() = default;
    ~MappedFile();

    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    MappedFile(MappedFile&& other) noexcept;
    MappedFile& operator=(MappedFile&& other) noexcept;

    [[nodiscard]] static core::Result<MappedFile> open(const std::string& path);

    [[nodiscard]] core::ByteSpan span() const noexcept { return view_; }
    [[nodiscard]] std::size_t size() const noexcept { return view_.size; }
    [[nodiscard]] bool empty() const noexcept { return view_.size == 0; }

private:
    void release() noexcept;

    core::ByteSpan view_{};
#if defined(_WIN32)
    void* file_handle_{nullptr};
    void* mapping_handle_{nullptr};
#else
    void* mapped_{nullptr};
    int fd_{-1};
#endif
};

} // namespace tessera::mmap
