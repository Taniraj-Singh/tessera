#pragma once

#include "tessera/core/types.hpp"
#include "tessera/model/container.hpp"
#include "tessera/textio/json.hpp"

#include <string>
#include <string_view>

namespace tessera::textio {

// Bidirectional JSON representation of a container. to_json renders the full
// model (header, per-section schema, and decoded rows); from_json rebuilds a
// model from that representation, validating field types and value shapes.
[[nodiscard]] JsonValue container_to_json(const model::Container& container);
[[nodiscard]] core::Result<model::Container> container_from_json(const JsonValue& root);

// Convenience wrappers over the string form.
[[nodiscard]] std::string container_to_json_string(const model::Container& container, bool pretty = true);
[[nodiscard]] core::Result<model::Container> container_from_json_string(std::string_view text);

} // namespace tessera::textio
