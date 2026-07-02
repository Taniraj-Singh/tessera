#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace tessera::memory {

// Fixed-type object pool with an intrusive free list. Objects are constructed
// in place inside pre-allocated chunks and recycled on release() without
// returning memory to the system, which keeps steady-state allocation flat for
// the churny short-lived nodes produced by the pipeline and recovery
// subsystems.
//
// acquire() returns a raw pointer whose lifetime the pool owns; release() runs
// the destructor and threads the slot back onto the free list. Chunk storage is
// never relocated, so outstanding pointers stay stable for the pool's lifetime.
template <typename T>
class ObjectPool {
public:
    static constexpr std::size_t kDefaultChunkSize = 64;

    ObjectPool() : ObjectPool(kDefaultChunkSize) {}
    explicit ObjectPool(std::size_t chunk_size)
        : chunk_size_(chunk_size == 0 ? kDefaultChunkSize : chunk_size) {}

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    ~ObjectPool() = default;

    template <typename... Args>
    [[nodiscard]] T* acquire(Args&&... args) {
        if (free_list_.empty()) {
            grow();
        }
        Slot* slot = free_list_.back();
        free_list_.pop_back();
        slot->live = true;
        ++live_count_;
        return new (static_cast<void*>(&slot->storage)) T(std::forward<Args>(args)...);
    }

    void release(T* object) {
        if (object == nullptr) {
            return;
        }
        object->~T();
        auto* slot = reinterpret_cast<Slot*>(object);
        slot->live = false;
        --live_count_;
        free_list_.push_back(slot);
    }

    [[nodiscard]] std::size_t live_count() const noexcept { return live_count_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return chunks_.size() * chunk_size_; }

private:
    struct alignas(T) Slot {
        std::aligned_storage_t<sizeof(T), alignof(T)> storage;
        bool live{false};
    };

    void grow() {
        auto chunk = std::make_unique<Slot[]>(chunk_size_);
        for (std::size_t index = chunk_size_; index > 0; --index) {
            free_list_.push_back(&chunk[index - 1]);
        }
        chunks_.push_back(std::move(chunk));
    }

    std::size_t chunk_size_{kDefaultChunkSize};
    std::size_t live_count_{0};
    std::vector<std::unique_ptr<Slot[]>> chunks_;
    std::vector<Slot*> free_list_;
};

} // namespace tessera::memory
