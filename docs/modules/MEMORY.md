# Memory Subsystem

`tessera::memory` provides the allocation and buffering primitives that the
encoder-side subsystems build on. It lives in the foundation layer alongside
`core`, `buffer`, and `checksum`, and depends only on `core` and `util`.

## Components

| Type | Header | Role |
|------|--------|------|
| `ByteBuffer` | `memory/byte_buffer.hpp` | Owning, geometrically growing byte sink with little-endian append helpers and in-place length backfilling (`patch_u32`). |
| `Arena` | `memory/arena.hpp` | Monotonic bump allocator over a chain of growing blocks; frees en masse on `reset()`. Aligned, non-relocating allocations. |
| `ObjectPool<T>` | `memory/object_pool.hpp` | Fixed-type free-list pool with in-place construction and slot recycling; stable pointers for the pool's lifetime. |

## Design notes

- **`ByteBuffer` vs `buffer::Writer`.** `Writer` performs *bounded* writes into
  borrowed memory and returns `Result<void>` on overrun. `ByteBuffer` *owns* its
  storage and grows, so its append helpers cannot overflow and return `void`.
  Both emit identical little-endian layouts, so a block can be produced by either
  and parsed by the same `buffer::Reader`.
- **Backfilling length prefixes.** Framed formats write a placeholder length,
  emit the payload, then `patch_u32` the real extent once known. `patch_u32`
  bounds-checks the offset and fails closed.
- **Arena alignment.** `allocate` rejects non-power-of-two alignments and
  reserves worst-case padding headroom when growing so the post-grow retry
  always fits. Pointers stay valid until `reset()`.
- **Pool recycling.** `release()` runs the destructor and threads the slot back
  onto the free list; the next `acquire()` reuses it. Chunk storage is never
  relocated.

## Consumers

The compression frame writer, archive writer, incremental encoder, and the
object-graph/pipeline node allocators all build on these primitives.
