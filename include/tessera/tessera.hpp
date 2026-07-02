#pragma once

// Foundation
#include "tessera/core/constants.hpp"
#include "tessera/core/types.hpp"
#include "tessera/encoding/varint.hpp"
#include "tessera/memory/byte_buffer.hpp"

// Artifact modules and model
#include "tessera/model/container.hpp"

// Data path
#include "tessera/archive/archive_reader.hpp"
#include "tessera/archive/archive_writer.hpp"
#include "tessera/compress/frame.hpp"
#include "tessera/convert/converter.hpp"
#include "tessera/parse/parser.hpp"
#include "tessera/pipeline/incremental_decoder.hpp"
#include "tessera/pipeline/incremental_encoder.hpp"
#include "tessera/pipeline/pipeline.hpp"
#include "tessera/serialize/serializer.hpp"
#include "tessera/stream/stream_reader.hpp"

// Analysis
#include "tessera/metadata/registry.hpp"
#include "tessera/schema/schema_compiler.hpp"
#include "tessera/schema/type_system.hpp"
#include "tessera/validate/constraint_validator.hpp"
#include "tessera/validate/semantic_validator.hpp"
#include "tessera/validate/validator.hpp"

// Lifecycle and introspection
#include "tessera/graph/object_graph.hpp"
#include "tessera/recovery/repair.hpp"
#include "tessera/resource/resource_manager.hpp"
#include "tessera/stats/statistics.hpp"
#include "tessera/txn/transaction.hpp"
#include "tessera/visit/visitor.hpp"

// Support
#include "tessera/config/config.hpp"
#include "tessera/diag/diagnostics.hpp"
#include "tessera/diag/trace.hpp"
#include "tessera/log/logger.hpp"
#include "tessera/mmap/mapped_file.hpp"
#include "tessera/plugins/registry.hpp"
#include "tessera/profile/profiler.hpp"
#include "tessera/version/negotiation.hpp"
