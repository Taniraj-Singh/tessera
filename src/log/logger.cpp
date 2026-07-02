#include "tessera/log/logger.hpp"

namespace tessera::log {

std::string_view level_name(Level level) noexcept {
    switch (level) {
    case Level::Trace:
        return "TRACE";
    case Level::Debug:
        return "DEBUG";
    case Level::Info:
        return "INFO";
    case Level::Warn:
        return "WARN";
    case Level::Error:
        return "ERROR";
    case Level::Off:
        return "OFF";
    }
    return "UNKNOWN";
}

void BufferSink::write(Level level, std::string_view message) {
    std::string line(level_name(level));
    line += ": ";
    line.append(message);
    lines_.push_back(std::move(line));
}

void Logger::add_sink(Sink& sink) {
    sinks_.push_back(&sink);
}

bool Logger::enabled(Level level) const noexcept {
    return level_ != Level::Off && static_cast<std::uint8_t>(level) >= static_cast<std::uint8_t>(level_);
}

void Logger::log(Level level, std::string_view message) {
    if (!enabled(level)) {
        return;
    }
    for (Sink* sink : sinks_) {
        sink->write(level, message);
    }
}

} // namespace tessera::log
