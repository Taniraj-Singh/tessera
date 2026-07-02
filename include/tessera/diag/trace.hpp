#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace tessera::diag {

enum class TraceEventKind : std::uint8_t {
    Enter,
    Exit,
    Mark,
};

struct TraceEvent {
    TraceEventKind kind{TraceEventKind::Mark};
    std::string name;
    std::uint64_t tick{0};
    std::uint32_t depth{0};
};

// Lightweight, deterministic tracer. Time is a logical tick that increments per
// event rather than a wall clock, so traces are reproducible across runs (and
// safe in environments without a clock). Events land in a fixed-capacity ring
// buffer; once full, the oldest events are overwritten.
class Tracer {
public:
    static constexpr std::size_t kDefaultCapacity = 1024;

    Tracer() : Tracer(kDefaultCapacity) {}
    explicit Tracer(std::size_t capacity);

    void enter(std::string name);
    void exit();
    void mark(std::string name);

    // Events in chronological order (oldest retained first).
    [[nodiscard]] std::vector<TraceEvent> events() const;

    [[nodiscard]] std::uint64_t tick() const noexcept { return tick_; }
    [[nodiscard]] std::uint32_t depth() const noexcept { return depth_; }
    [[nodiscard]] std::size_t recorded() const noexcept { return buffer_.size(); }

    void reset() noexcept;

private:
    void push(TraceEventKind kind, std::string name, std::uint32_t depth);

    std::vector<TraceEvent> buffer_;
    std::size_t capacity_{kDefaultCapacity};
    std::size_t head_{0}; // index of the oldest event once wrapped
    bool wrapped_{false};
    std::uint64_t tick_{0};
    std::uint32_t depth_{0};
};

// RAII span: enters on construction, exits on scope end. Depth-balanced.
class ScopedSpan {
public:
    ScopedSpan(Tracer& tracer, std::string name) : tracer_(&tracer) { tracer_->enter(std::move(name)); }
    ~ScopedSpan() { tracer_->exit(); }

    ScopedSpan(const ScopedSpan&) = delete;
    ScopedSpan& operator=(const ScopedSpan&) = delete;

private:
    Tracer* tracer_;
};

} // namespace tessera::diag
