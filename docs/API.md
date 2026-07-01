# Tessera API Overview

## Umbrella Header

```cpp
#include <tessera/tessera.hpp>
```

## Core

| Symbol | Description |
|--------|-------------|
| `core::Result<T>` | Success value or structured `Error` |
| `core::ByteSpan` | Non-owning read-only byte view |
| `core::kContainerMagic` | TBC magic constant (`TBCS`) |
| `core::kContainerVersion` | Current format version (`1.0`) |

## Parse / Serialize

```cpp
tessera::parse::ParseOptions options;
auto container = tessera::parse::parse_container(input, options);

auto bytes = tessera::serialize::serialize_container(*container, {});
```

## Validation

```cpp
auto report = tessera::validate::validate_bytes(input, {});
if (!report.valid) { /* inspect report.diagnostics */ }
```

## Streaming

```cpp
tessera::stream::StreamReader reader(input);
while (!reader.finished()) {
    auto event = reader.next();
}
```

## Memory Mapping

```cpp
auto mapped = tessera::mmap::MappedFile::open("file.tbc");
auto input = mapped->span();
```

## Version Negotiation

```cpp
tessera::version::VersionRange range{0x0100, 0x0100};
tessera::version::negotiate(container_version, range);
```

## Conversion

```cpp
tessera::convert::ConvertOptions opts;
opts.format = tessera::convert::OutputFormat::JsonSummary;
auto text = tessera::convert::convert_container(container, opts);
```

## Plugins

```cpp
tessera::plugins::Registry::instance().register_plugin(my_plugin);
```

Hook points: `AfterHeader`, `AfterCatalog`, `AfterSchema`, `AfterRecords`, `BeforeSerialize`.

## CLI Tools

| Tool | Purpose |
|------|---------|
| `tbc_inspect` | Parse and inspect containers (`--json`, `--validate`, `--stream`) |
| `tbc_convert` | Convert to JSON, hex dump, or canonical binary |
