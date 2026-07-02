#include "tessera/diag/trace.hpp"

namespace tessera::diag {

Tracer::Tracer(std::size_t capacity) : capacity_(capacity == 0 ? kDefaultCapacity : capacity) {
    buffer_.reserve(capacity_);
}

void Tracer::push(TraceEventKind kind, std::string name, std::uint32_t depth) {
    TraceEvent event{kind, std::move(name), tick_++, depth};
    if (buffer_.size() < capacity_) {
        buffer_.push_back(std::move(event));
        return;
    }
    // Ring is full: overwrite the oldest slot and advance the head.
    buffer_[head_] = std::move(event);
    head_ = (head_ + 1U) % capacity_;
    wrapped_ = true;
}

void Tracer::enter(std::string name) {
    push(TraceEventKind::Enter, std::move(name), depth_);
    ++depth_;
}

void Tracer::exit() {
    if (depth_ > 0) {
        --depth_;
    }
    push(TraceEventKind::Exit, "", depth_);
}

void Tracer::mark(std::string name) {
    push(TraceEventKind::Mark, std::move(name), depth_);
}

std::vector<TraceEvent> Tracer::events() const {
    if (!wrapped_) {
        return buffer_;
    }
    std::vector<TraceEvent> ordered;
    ordered.reserve(buffer_.size());
    for (std::size_t step = 0; step < buffer_.size(); ++step) {
        ordered.push_back(buffer_[(head_ + step) % buffer_.size()]);
    }
    return ordered;
}

void Tracer::reset() noexcept {
    buffer_.clear();
    head_ = 0;
    wrapped_ = false;
    tick_ = 0;
    depth_ = 0;
}

} // namespace tessera::diag
