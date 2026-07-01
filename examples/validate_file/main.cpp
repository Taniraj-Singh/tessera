#include "tessera/tessera.hpp"

#include <iostream>
#include <vector>

class TimestampPlugin final : public tessera::plugins::Plugin {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "timestamp-plugin"; }

    tessera::core::Result<void> on_hook(tessera::plugins::HookPoint point, tessera::model::Container& container) override {
        if (point == tessera::plugins::HookPoint::AfterHeader && container.header.created_unix_ms == 0) {
            return tessera::core::Error{tessera::core::ErrorCode::PluginRejected, "created timestamp required", 0};
        }
        return tessera::core::Ok();
    }
};

int main() {
    TimestampPlugin plugin{};
    tessera::plugins::Registry::instance().register_plugin(plugin);

    tessera::model::Container container{};
    container.header.magic = tessera::core::kContainerMagic;
    container.header.version = tessera::core::kContainerVersion;
    container.header.created_unix_ms = 123456789ULL;
    container.header.section_count = 0;

    if (auto bytes = tessera::serialize::serialize_container(container, {}); bytes) {
        if (auto parsed = tessera::parse::parse_container(tessera::core::ByteSpan{bytes->data(), bytes->size()}, {}); parsed) {
            std::cout << "Plugin-enabled round-trip succeeded\n";
            return 0;
        }
    }

    std::cerr << "plugin example failed\n";
    return 1;
}
