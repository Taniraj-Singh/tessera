#include "tessera/resource/resource_manager.hpp"

#include <utility>

namespace tessera::resource {

core::Result<void> ResourceManager::acquire(std::size_t bytes) {
    if (budget_.max_handles != 0 && handles_ + 1U > budget_.max_handles) {
        return core::Error{core::ErrorCode::InvalidLength, "resource handle budget exhausted", handles_};
    }
    if (budget_.max_bytes != 0) {
        // Guard against wraparound before comparing against the cap.
        if (used_bytes_ + bytes < used_bytes_ || used_bytes_ + bytes > budget_.max_bytes) {
            return core::Error{core::ErrorCode::InvalidLength, "resource byte budget exhausted", used_bytes_};
        }
    }
    used_bytes_ += bytes;
    ++handles_;
    return core::Ok();
}

void ResourceManager::release(std::size_t bytes) noexcept {
    used_bytes_ = bytes > used_bytes_ ? 0 : used_bytes_ - bytes;
    if (handles_ > 0) {
        --handles_;
    }
}

bool ResourceManager::within_budget() const noexcept {
    if (budget_.max_bytes != 0 && used_bytes_ > budget_.max_bytes) {
        return false;
    }
    if (budget_.max_handles != 0 && handles_ > budget_.max_handles) {
        return false;
    }
    return true;
}

core::Result<Lease> Lease::take(ResourceManager& manager, std::size_t bytes) {
    if (auto status = manager.acquire(bytes); !status) {
        return status.error();
    }
    return Lease(manager, bytes);
}

Lease::Lease(Lease&& other) noexcept : manager_(other.manager_), bytes_(other.bytes_) {
    other.manager_ = nullptr;
    other.bytes_ = 0;
}

Lease& Lease::operator=(Lease&& other) noexcept {
    if (this != &other) {
        release();
        manager_ = other.manager_;
        bytes_ = other.bytes_;
        other.manager_ = nullptr;
        other.bytes_ = 0;
    }
    return *this;
}

Lease::~Lease() {
    release();
}

void Lease::release() noexcept {
    if (manager_ != nullptr) {
        manager_->release(bytes_);
        manager_ = nullptr;
        bytes_ = 0;
    }
}

} // namespace tessera::resource
