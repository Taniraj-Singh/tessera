# Compression Subsystem

`tessera::compress` is a pluggable codec layer plus a self-describing frame
envelope. It sits above the foundation and below the artifact modules; sections
may eventually declare a codec so their payloads are stored compressed.

## Codecs

All codecs implement `compress::Codec` and are reachable as process-wide
singletons through `find_codec(CodecId)`.

| Codec | Id | Scheme |
|-------|----|--------|
| `StoreCodec` | 0 | Identity. `decompress` verifies the stored length equals the declared size. |
| `RleCodec` | 1 | PackBits-style byte runs (literal runs ≤128, repeat runs ≤129). |
| `Lz77Codec` | 2 | LZSS sliding window, 12-bit distance (≤4096) / 4-bit length (3–18), tag-byte grouping. |

Decompression always takes the **declared uncompressed size**. Every codec:

- pre-sizes its output to that size;
- checks each token against the remaining budget *before* applying it;
- fails closed unless it produces exactly the declared number of bytes.

For LZ77 specifically, a back-reference must name a distance no larger than the
output produced so far, and a copy must not push the output past the declared
size. These are the invariants the `compress_fuzzer` probes.

## Frames

`compress_frame` / `decompress_frame` wrap a codec payload:

```
u32 magic | u8 codec | u8 flags | uvarint uncompressed | uvarint compressed | payload | u32 crc32c
```

The reader validates the magic, bounds both declared sizes against
`core::kMaxBlockSize`, verifies the CRC over the payload, and only then invokes
the codec. `compress_frame` falls back to `Store` when a transform fails to
shrink the input, so a frame never costs more than the raw bytes plus a small
header.

## Fuzzing

`fuzz/compress_fuzzer.cpp` has two entry paths: a direct codec-decompress path
(attacker-chosen declared size, no magic/CRC gate) for maximum reach into the
back-reference logic, and a full-frame path for the envelope.
