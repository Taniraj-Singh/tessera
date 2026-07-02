#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tessera::log {

enum class Level : std::uint8_t {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Off,
};

[[nodiscard]] std::string_view level_name(Level level) noexcept;

// Output destination for a logger. Sinks receive already-formatted messages that
// passed the level filter.
class Sink {
public:
    virtual ~Sink() = default;
    virtual void write(Level level, std::string_view message) = 0;
};

// Captures log lines in memory — useful for tests and for surfacing recent
// activity in diagnostics.
class BufferSink final : public Sink {
public:
    void write(Level level, std::string_view message) override;

    [[nodiscard]] const std::vector<std::string>& lines() const noexcept { return lines_; }
    [[nodiscard]] std::size_t size() const noexcept { return lines_.size(); }
    void clear() noexcept { lines_.clear(); }

private:
    std::vector<std::string> lines_;
};

// Leveled logger fanning out to registered sinks. Messages below the active
// level are dropped before formatting. Sinks are borrowed, not owned.
class Logger {
public:
    void set_level(Level level) noexcept { level_ = level; }
    [[nodiscard]] Level level() const noexcept { return level_; }

    void add_sink(Sink& sink);
    [[nodiscard]] bool enabled(Level level) const noexcept;

    void log(Level level, std::string_view message);

    void trace(std::string_view message) { log(Level::Trace, message); }
    void debug(std::string_view message) { log(Level::Debug, message); }
    void info(std::string_view message) { log(Level::Info, message); }
    void warn(std::string_view message) { log(Level::Warn, message); }
    void error(std::string_view message) { log(Level::Error, message); }

private:
    Level level_{Level::Info};
    std::vector<Sink*> sinks_;
};

} // namespace tessera::log
