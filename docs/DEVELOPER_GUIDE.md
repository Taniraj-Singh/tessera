# Developer Guide

## Prerequisites

- CMake 3.20+
- C++20 compiler (GCC 11+, Clang 14+, or MSVC 19.30+)
- Optional: Clang with libFuzzer for fuzz targets

## Build Configurations

| Flag | Default | Purpose |
|------|---------|---------|
| `TESSERA_BUILD_TESTS` | ON | Unit/integration/regression/edge tests |
| `TESSERA_BUILD_EXAMPLES` | ON | Sample programs |
| `TESSERA_BUILD_TOOLS` | ON | CLI utilities |
| `TESSERA_BUILD_FUZZ` | OFF | libFuzzer harnesses (Clang only) |
| `TESSERA_ENABLE_SANITIZERS` | OFF | ASan + UBSan |

## Project Workflow

1. Edit headers under `include/tessera/`
2. Implement sources under `src/<module>/`
3. Add tests in `tests/unit`, `tests/integration`, etc.
4. Register new sources in `src/CMakeLists.txt`

## Error Handling Convention

Use `tessera::core::Result<T>` for all parse/serialize operations:

```cpp
if (auto header = tessera::header::parse(input); header) {
    // use header.value()
} else {
    // header.error().code, .message, .offset
}
```

Do not throw exceptions on malformed input.

## Adding a Plugin

```cpp
class MyPlugin final : public tessera::plugins::Plugin {
public:
    std::string_view name() const noexcept override { return "my-plugin"; }
    tessera::core::Result<void> on_hook(tessera::plugins::HookPoint point,
                                         tessera::model::Container& container) override {
        return tessera::core::Ok();
    }
};
```

Register before parse/serialize:

```cpp
MyPlugin plugin;
tessera::plugins::Registry::instance().register_plugin(plugin);
```

## Running Tests

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## ClusterFuzzLite

The `.clusterfuzzlite/` directory contains `build.sh` and `project.yaml`. The build script produces four fuzz targets into `$OUT`.

## Coding Standards

- C++20, no extensions
- RAII and move semantics throughout
- Const-correct interfaces
- One class per responsibility
- Warnings treated as errors on GCC/Clang
