#pragma once

#include "tessera/core/types.hpp"
#include "tessera/model/container.hpp"

#include <string>
#include <vector>

namespace tessera::plugins {

enum class HookPoint {
    AfterHeader,
    AfterCatalog,
    AfterSchema,
    AfterRecords,
    BeforeSerialize,
};

class Plugin {
public:
    virtual ~Plugin() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    virtual core::Result<void> on_hook(HookPoint point, model::Container& container) = 0;
};

class Registry {
public:
    static Registry& instance();

    void register_plugin(Plugin& plugin);
    void unregister_plugin(Plugin& plugin);

    [[nodiscard]] core::Result<void> run_hooks(HookPoint point, model::Container& container) const;

private:
    std::vector<Plugin*> plugins_;
};

} // namespace tessera::plugins
