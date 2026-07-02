# Index & Hash Subsystems

`tessera::hash` and `tessera::index` add O(1) lookup and introspection over a
container's sections, replacing the linear scans that `validate` and `convert`
performed.

## Hashing (`tessera::hash`)

| Function | Header | Use |
|----------|--------|-----|
| `fnv1a32` / `fnv1a64` | `hash/fnv.hpp` | String keys (section/field names). Full key is stored and compared on lookup, so collisions are correctness-neutral. |
| `murmur3_x86_32` | `hash/murmur.hpp` | Bulk byte hashing. |
| `fmix32` / `fmix64` | `hash/murmur.hpp` | Avalanche finalizers used to spread integer keys before masking. |

## Indexes (`tessera::index`)

- **`HashIndex`** — open-addressing `uint64 → uint32` map. Linear probing with
  tombstones, power-of-two capacity, growth at ~0.69 load. `find` returns a
  pointer or `nullptr`; no allocation on the lookup path.
- **`StringIndex`** — open-addressing `string → uint32` map that stores the full
  key so hash collisions between distinct strings are resolved by comparison.
- **`SectionIndex`** — a snapshot facade combining both: `find_by_id` and
  `find_by_name` resolve a section to its slot in `Container::sections`. Rebuild
  after structural mutation; it does not track changes.

### Probe termination

Both tables keep the live-plus-tombstone occupancy below capacity, guaranteeing
every probe chain contains at least one `Empty` slot, so `find` on an absent key
always terminates without scanning the whole table.
