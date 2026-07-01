#include "tessera/plugins/registry.hpp"

#include <algorithm>

namespace tessera::plugins {

Registry& Registry::instance() {
    static Registry registry;
    return registry;
}

void Registry::register_plugin(Plugin& plugin) {
    plugins_.push_back(&plugin);
}

void Registry::unregister_plugin(Plugin& plugin) {
    plugins_.erase(
        std::remove(plugins_.begin(), plugins_.end(), &plugin),
        plugins_.end());
}

core::Result<void> Registry::run_hooks(HookPoint point, model::Container& container) const {
    for (Plugin* plugin : plugins_) {
        if (auto status = plugin->on_hook(point, container); !status) {
            core::Error err = status.error();
            err.code = core::ErrorCode::PluginRejected;
            err.message = std::string(plugin->name()) + ": " + err.message;
            return err;
        }
    }
    return core::Ok();
}

} // namespace tessera::plugins
